#ifndef lint
static  char    *sccsid = "@(#)tcp_input.c	4.9	(ULTRIX)	3/8/91";
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
 *      03-Mar-91 - Michael G. Mc Menemy
 *      Fix oob code to handle both XTI and Berkeley style oob data.
 *
 *      03-Mar-91 - Michael G. Mc Menemy
 *      Fix rexmt when all outstanding packets have been acked.
 * 
 *      03-Mar-91 - Michael G. Mc Menemy
 *      Fix unaligned access panic in oob code.
 * 
 *	16-Jan-91 - U. Sinkewicz
 *	Fix to stack overwrite panic in LOOPBACK mode from tcp_output()
 *	and tcp_input() thrashing.
 *
 *	09-oct-90 - jaw
 *	merge in mm fix for deadlock caused by bug in tcp_input.
 *
 *	17-Jul-90  - jsd
 *	remove so->ref panic since it's optimized out for single-cpu
 *
 *	16-July-90 - lp
 *	Leave trailers for machines that use them (IFF_NOTRAILERS).
 *
 *	18-June-90 -  lp
 *	Remove trailers in max seg calculation for FDDI. Note that
 * 	ethernet will now no longer do trailers encapsulation.
 *
 *	14-May-90 - R. Bhanukitsiri
 *	Fix "m_free has bad m_cltype" panic by not freeing up old
 *	out-of-band data when a new out-of-band data arrives.
 *
 *	26-Jan-90 - gmm
 *	Make socket pointer in tcp_input() volatile
 *
 *	3-Jan-90  - U. Sinkewicz
 *	Performance enhancements to uniprocessor kernel.
 *
 *	2-Dec-89  - jsd
 *	Panic during so->ref check for single-cpu so we don't hang
 *
 *	9-Nov-89  - Ursula Sinkewicz
 *	Fix to the case when we have a SYN in the window and are
 *	responding with an RST.
 *
 *	30-May-89 - Ursula Sinkewicz
 *	Added so->ref field which is used in conjunction with SO_LOCK
 *	to fix smp hole caused by unlocking the socket and locking it
 *	again.  Problem is that the socket may have changed or been
 *	freed while unlocked.  The unlocking is necessary to accommodate
 *	the lock heirarchy and sleeps.
 *
 *      05-05-89 - Michael G. Mc Menemy
 *              Add XTI support.
 *
 *	27-2-89 - Ursula Sinkewicz
 *		Lowered ipl on lk_rtentry, lk_ifnet, lk_in_ifaddr;
 *		replace tcpstatictics with a macro, as per lp changes 
 *		from 3/16/89.
 *
 *	3-3-89 - Ursula Sinkewicz
 *		Added support for new directory layout to smp file.
 *
 *	2-23-89 - Ursula Sinkewicz
 *		Spool dangling reference info to error logger.  Added
 * 		change to eleminate willbefreed flag which coincides
 *		with uipc_socket.c changes.
 *
 *	12-5-88	- Ursula Sinkewicz
 *		SMP - release memory for socket/pcb on close/shutdown/
 *		abort.	
 *
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes.
 *									*
 *	6/25/87 - Ursula Sinkewicz					*
 *		Major cleanup of freeondrop flag.			*
 *
 *	04/27/87 - Larry Cohen
 *		Send response to everything except a reset.  Code 
 *		effected is right after "dropafterack:"
 *									
 *	01/28/87 - Larry Cohen
 *		Look for resets before checking to see if input is
 *		within acceptable range.  This is because a response
 *		to KEEPALIVE messages is not within an acceptable range.
 *		If the response to our KEEPALIVE message is a reset
 *		(which is the case if the remote side has crashed and 
 *		come up again soon) we would otherwise drop the RST 
 *		message and leave local processes hanging.
 *
 *	01/28/87 - Larry Cohen
 *		update max_sndwnd
 * 	12/16/86 - lp 							*
 *		Start timer on FIN_WAIT_2 state if CANTRECVMORE 	*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	tcp_input.c	7.13 (Berkeley) 11/13/87
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/smp_lock.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/net/if.h"
#include "../net/net/route.h"

#include "../net/netinet/in.h"
#include "../net/netinet/in_pcb.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"
#include "../net/netinet/tcp.h"
#include "../net/netinet/tcp_fsm.h"
#include "../net/netinet/tcp_seq.h"
#include "../net/netinet/tcp_timer.h"
#include "../net/netinet/tcp_var.h"
#include "../net/netinet/tcpip.h"
#include "../net/netinet/tcp_debug.h"

#ifdef XTI
extern	char *tcpstates[];
extern  int xti_debug; /* general debug */
#define PRINTXTID(level, msg)   \
  /*                            \
   * level:                     \
   * 0x01 = generate events;    \
   * 0x04 = acceptchk/abort support;  \
   * 0x08 = peek events;        \
   * 0x10 = tpdu T_MORE support; \
   * 0x20 = oob mark;           \
   * 0x40 = option neg.         \
   * 0x80 = protocol;           \
   */                           \
  if (xti_debug & (level))      \
     cprintf((msg))
#else
#define PRINTXTID(level, msg)
#endif XTI

int	tcpprintfs = 0;
int	tcpcksum = 1;
int	tcprexmtthresh = 3;
struct	tcpiphdr tcp_saveti;
extern	tcpnodelack;

struct	tcpcb *tcp_newtcpcb();

/*
 * Insert segment ti into reassembly queue of tcp with
 * control block tp.  Return TH_FIN if reassembly now includes
 * a segment with FIN.  The macro form does the common case inline
 * (segment is the next to be received on an established connection,
 * and the queue is empty), avoiding linkage into and removal
 * from the queue and repetition of various conversions.
 */
#ifdef XTI
#define	TCP_REASS(tp, ti, m, so, flags) { \
	if ((ti)->ti_seq == (tp)->rcv_nxt && \
	    (tp)->seg_next == (struct tcpiphdr *)(tp) && \
	    (tp)->t_state == TCPS_ESTABLISHED) { \
		(tp)->rcv_nxt += (ti)->ti_len; \
		flags = (ti)->ti_flags & TH_FIN; \
		TCPSTAT(tcps_rcvpack++);\
		TCPSTAT(tcps_rcvbyte += (ti)->ti_len);\
/*#ifdef XTI */\
	          if ((so)->so_xticb.xti_epvalid) { \
                    if ((so)->so_xticb.xti_evtenabled) { \
                      if (mbuf_any_len((m)) > 0) { \
		      (so)->so_xticb.xti_evtarray[XTI_EVT_T_DATA]++; \
		      PRINTXTID(1, ("T_DATA (#1) 1 event generated\n")); \
		      }}} \
/*#endif XTI */\
		sbappend(&(so)->so_rcv, (m)); \
		sorwakeup(so); \
	} else \
		(flags) = tcp_reass((tp), (ti)); \
}
#else
#define	TCP_REASS(tp, ti, m, so, flags) { \
	if ((ti)->ti_seq == (tp)->rcv_nxt && \
	    (tp)->seg_next == (struct tcpiphdr *)(tp) && \
	    (tp)->t_state == TCPS_ESTABLISHED) { \
		(tp)->rcv_nxt += (ti)->ti_len; \
		flags = (ti)->ti_flags & TH_FIN; \
		TCPSTAT(tcps_rcvpack++);\
		TCPSTAT(tcps_rcvbyte += (ti)->ti_len);\
		sbappend(&(so)->so_rcv, (m)); \
		sorwakeup(so); \
	} else \
		(flags) = tcp_reass((tp), (ti)); \
}
#endif XTI

tcp_reass(tp, ti)
	register struct tcpcb *tp;
	register struct tcpiphdr *ti;
{
	register struct tcpiphdr *q;
	struct socket *so = tp->t_inpcb->inp_socket;
	struct mbuf *m;
	int flags;

	/*
	 * Call with ti==0 after become established to
	 * force pre-ESTABLISHED data up to user socket.
	 */
	if (ti == 0)
		goto present;

	/*
	 * Find a segment which begins after this one does.
	 */
	for (q = tp->seg_next; q != (struct tcpiphdr *)tp;
	    q = (struct tcpiphdr *)q->ti_next)
		if (SEQ_GT(q->ti_seq, ti->ti_seq))
			break;

	/*
	 * If there is a preceding segment, it may provide some of
	 * our data already.  If so, drop the data from the incoming
	 * segment.  If it provides all of our data, drop us.
	 */
	if ((struct tcpiphdr *)q->ti_prev != (struct tcpiphdr *)tp) {
		register int i;
		q = (struct tcpiphdr *)q->ti_prev;
		/* conversion to int (in i) handles seq wraparound */
		i = q->ti_seq + q->ti_len - ti->ti_seq;
		if (i > 0) {
			if (i >= ti->ti_len) {
				TCPSTAT(tcps_rcvduppack++);
				TCPSTAT(tcps_rcvdupbyte += ti->ti_len);
				goto drop;
			}
			m_adj(dtom(ti), i);
			ti->ti_len -= i;
			ti->ti_seq += i;
		}
		q = (struct tcpiphdr *)(q->ti_next);
	}
	TCPSTAT(tcps_rcvoopack++);
	TCPSTAT(tcps_rcvoobyte += ti->ti_len);

	/*
	 * While we overlap succeeding segments trim them or,
	 * if they are completely covered, dequeue them.
	 */
	while (q != (struct tcpiphdr *)tp) {
		register int i = (ti->ti_seq + ti->ti_len) - q->ti_seq;
		if (i <= 0)
			break;
		if (i < q->ti_len) {
			q->ti_seq += i;
			q->ti_len -= i;
			m_adj(dtom(q), i);
			break;
		}
		q = (struct tcpiphdr *)q->ti_next;
		m = dtom(q->ti_prev);
		remque(q->ti_prev);
		m_freem(m);
	}

	/*
	 * Stick new segment in its place.
	 */
	insque(ti, q->ti_prev);

present:
	/*
	 * Present data to user, advancing rcv_nxt through
	 * completed sequence space.
	 */
	if (TCPS_HAVERCVDSYN(tp->t_state) == 0)
		return (0);
	ti = tp->seg_next;
	if (ti == (struct tcpiphdr *)tp || ti->ti_seq != tp->rcv_nxt)
		return (0);
	if (tp->t_state == TCPS_SYN_RECEIVED && ti->ti_len)
		return (0);
	do {
		tp->rcv_nxt += ti->ti_len;
		flags = ti->ti_flags & TH_FIN;
		remque(ti);
		m = dtom(ti);
		ti = (struct tcpiphdr *)ti->ti_next;
		if (so->so_state & SS_CANTRCVMORE){
			m_freem(m);
		}
#ifdef XTI
		else {
		  sbappend(&so->so_rcv, m);
		  if (so->so_xticb.xti_epvalid)  
		    if (so->so_xticb.xti_evtenabled) {
		      if (ti->ti_len) {
			so->so_xticb.xti_evtarray[XTI_EVT_T_DATA] ++;
			PRINTXTID(1, ("T_DATA (#2) 1 event generated\n"));
		      }
		    }
		}

#else
		else
			sbappend(&so->so_rcv, m);
#endif XTI
	} while (ti != (struct tcpiphdr *)tp && ti->ti_seq == tp->rcv_nxt);
	sorwakeup(so);
	return (flags);
drop:
	m_freem(dtom(ti));
return (0);
}

/*
 * TCP input routine, follows pages 65-76 of the
 * protocol specification dated September, 1981 very closely.
 */
/*
 * SMP: enter without socket lock set.  Come into the routine only
 * through the interrupt handler and in LOOPBACK mode.
 */

tcp_input(m0)
	struct mbuf *m0;
{
	register struct tcpiphdr *ti;
	struct inpcb *inp;
	struct inpcb *inp_temp;
	register struct mbuf *m;
	struct mbuf *om = 0;
	int len, tlen, off;
	register struct tcpcb *tp = 0;
	register int tiflags;
	int todrop, acked, ourfinisacked, needoutput = 0;
	volatile struct socket *so = NULL;  /* SMP */
	short ostate;
	struct in_addr laddr;
	int dropsocket = 0;
	int iss = 0;
	int freeondrop = 1; 		/* SMP */
	struct socket *so_tmp; 		/* SMP */
	int status = 0;			/* SMP */
	struct socket *so_addr = NULL;	/* SMP */
	int owner = 0;			/* SMP */
	int error = 0;
	int waitcnt; /*SMP*/
#ifdef XTI
	short xti_precedence;
	struct secoptions xti_secopt;
#endif XTI

	TCPSTAT(tcps_rcvtotal++);
	/*
	 * Get IP and TCP header together in first mbuf.
	 * Note: IP leaves IP header in first mbuf.
	 */
	m = m0;
	ti = mtod(m, struct tcpiphdr *);
#ifdef XTI
	/* get precedence from IP header */

	xti_precedence = (((struct ip *)ti)->ip_tos & (0x07));

	/* get IP Security options */

	if (xti_ip_dooptions(((struct ip *) ti), &xti_secopt)) {
	  xti_secopt.security = T_UNUSED;
	  xti_secopt.compartment = T_UNUSED;
	  xti_secopt.handling = T_UNUSED;
	  xti_secopt.tcc = T_UNUSED;
	}

#endif XTI
	if (((struct ip *)ti)->ip_hl > (sizeof (struct ip) >> 2))
		ip_stripoptions((struct ip *)ti, (struct mbuf *)0);
	if (m->m_off > MMAXOFF || m->m_len < sizeof (struct tcpiphdr)) {
		if ((m = m_pullup(m, sizeof (struct tcpiphdr))) == 0) {
			TCPSTAT(tcps_rcvshort++);
			return;
		}
		ti = mtod(m, struct tcpiphdr *);
	}

	/*
	 * Checksum extended TCP header and data.
	 */
	tlen = ((struct ip *)ti)->ip_len;
	len = sizeof (struct ip) + tlen;
#ifdef TCPLOOPBACK
	if (tcpcksum && (ti->ti_src.s_addr != ti->ti_dst.s_addr)) {
#else
	if (tcpcksum) {
#endif TCPLOOPBACK
		ti->ti_next = ti->ti_prev = 0;
		ti->ti_x1 = 0;
		ti->ti_len = (u_short)tlen;
		ti->ti_len = htons((u_short)ti->ti_len);
		if (ti->ti_sum = in_cksum(m, len)) {
			if (tcpprintfs)
				printf("tcp sum: src %x\n", ti->ti_src);
			TCPSTAT(tcps_rcvbadsum++);
			freeondrop = 0;
			goto drop;
		}
	}

	/*
	 * Check that TCP offset makes sense,
	 * pull out TCP options and adjust length.
	 */
	off = ti->ti_off << 2;
	if (off < sizeof (struct tcphdr) || off > tlen) {
		if (tcpprintfs)
			printf("tcp off: src %x off %d\n", ti->ti_src, off);
		TCPSTAT(tcps_rcvbadoff++);
		freeondrop = 0;
		goto drop;
	}
	tlen -= off;
	ti->ti_len = tlen;
	if (off > sizeof (struct tcphdr)) {
		if (m->m_len < sizeof(struct ip) + off) {
			if ((m = m_pullup(m, sizeof (struct ip) + off)) == 0) {
				TCPSTAT(tcps_rcvshort++);
				return;
			}
			ti = mtod(m, struct tcpiphdr *);
		}
		om = m_get(M_DONTWAIT, MT_DATA);
		if (om == 0) {
			freeondrop = 0;
			goto drop;
		}
		om->m_len = off - sizeof (struct tcphdr);
		{ caddr_t op = mtod(m, caddr_t) + sizeof (struct tcpiphdr);
		  bcopy(op, mtod(om, caddr_t), (unsigned)om->m_len);
		  m->m_len -= om->m_len;
		  bcopy(op+om->m_len, op,
		   (unsigned)(m->m_len-sizeof (struct tcpiphdr)));
		}
	}
	tiflags = ti->ti_flags;

	/*
	 * Drop TCP and IP headers; TCP options were dropped above.
	 */
	m->m_off += sizeof(struct tcpiphdr);
	m->m_len -= sizeof(struct tcpiphdr);

	/*
	 * Convert TCP protocol specific fields to host format.
	 */
	ti->ti_seq = ntohl(ti->ti_seq);
	ti->ti_ack = ntohl(ti->ti_ack);
	ti->ti_win = ntohs(ti->ti_win);
	ti->ti_urp = ntohs(ti->ti_urp);

	/*
	 * Locate pcb for segment.
	 */
	/* 4.13.89.us  You need to start with an owner check on
	 * the tcb because there is an obscure path here from
	 * the slow timers which set lk_tcb.
	 */
findpcb:
	if ( smp && !(smp_owner(&lk_tcb)) ){
		owner = 1;
		smp_lock(&lk_tcb, LK_RETRY);
	}
	else 
		owner = 0;
	inp = in_pcblookup
		(&tcb, ti->ti_src, ti->ti_sport, ti->ti_dst, ti->ti_dport,
		INPLOOKUP_WILDCARD);
loop1:	if (inp == 0) {
		freeondrop = 0;
		tp = 0;
		so = NULL;	/* This is for tcp_respond() locking.  */
		if ( owner){
			smp_unlock(&lk_tcb);
			owner = 0;
		}
		goto dropwithreset;
	}
	so = inp->inp_socket;
	if (smp){

	  /* try only once to avoid DEADLOCK.  we have lk_tcb and need to 
	     get it released as soon as possible. */
	  if (smp_lock(&so->lk_socket, LK_ONCE) || 
	      smp_lock(&so->lk_socket, LK_ONCE)) {
	  	if (so->ref > 0){	/* if some one owns it then backoff */
			smp_unlock(&so->lk_socket);
			smp_unlock(&lk_tcb);
			waitcnt = 1000000; /* need some limit to wait */
			while (so->ref > 0 && waitcnt--) 
				;
			if (owner) 
				owner=0;
			else 
				smp_lock(&lk_tcb,LK_RETRY);
			goto findpcb;		
		}	  	
	  } else {
	  	/* didn't get lock on socket.... still back off if referenced*/
		smp_unlock(&lk_tcb);
		waitcnt = 1000000;
		while (so->ref > 0 && waitcnt--) ;
		if (owner) 
			owner=0;
		else 
			smp_lock(&lk_tcb,LK_RETRY);
		goto findpcb;
	  }
	  inp_temp = in_pcblookup
	    (&tcb, ti->ti_src, ti->ti_sport, ti->ti_dst, ti->ti_dport,
	    INPLOOKUP_WILDCARD);
	  if (inp != inp_temp) {
		freeondrop = 0;
		smp_unlock(&so->lk_socket);
		/* We are unlocked so don't keep a messy socket pointer
		 * around because it could confuse tcp_respond.  */
		so = NULL;
		if (owner){
			smp_unlock(&lk_tcb);
			owner = 0;
		}
		goto dropwithreset;
	  }
	  if ( owner){
		smp_unlock(&lk_tcb);
	  }
	}
	/*
	 * If the state is CLOSED (i.e., TCB does not exist) then
	 * all data in the incoming segment is discarded.
	 * If the TCB exists but is in CLOSED state, it is embryonic,
	 * but should either do a listen or a connect soon.
	 */
	tp = intotcpcb(inp);
	if (tp == 0)
		goto dropwithreset;
#ifdef XTI
	PRINTXTID(128, ("tcp_input: tp =%x,tp->t_state = %s\n",tp,tcpstates[tp->t_state]));
	if (ti->ti_flags) 
	  PRINTXTID(128, ("<"));
	if (ti->ti_flags & TH_FIN) 
	  PRINTXTID(128, ("FIN "));
	if (ti->ti_flags & TH_SYN) 
	  PRINTXTID(128, ("SYN "));
	if (ti->ti_flags & TH_RST) 
	  PRINTXTID(128, ("RST "));
	if (ti->ti_flags & TH_PUSH) 
	  PRINTXTID(128, ("PSH "));
	if (ti->ti_flags & TH_ACK) 
	  PRINTXTID(128, ("ACK "));
	if (ti->ti_flags & TH_URG) 
	  PRINTXTID(128, ("URG "));
	if (ti->ti_flags) 
	  PRINTXTID(128, (">\n"));
#endif XTI
	if (tp->t_state == TCPS_CLOSED)
		goto drop; 
	if (so->so_options & SO_DEBUG) {
		ostate = tp->t_state;
		tcp_saveti = *ti;
	}
	/* 
	 * Unlock the old and lock the new...
	 */
	if (so->so_options & SO_ACCEPTCONN) {
		so_tmp = sonewconn(so);
		if (so_tmp == 0) {
			freeondrop = 0;
			goto drop;
		}
		SO_LOCK(so_tmp);
		smp_unlock(&so->lk_socket);
		so = so_tmp;
		/*
		 * This is ugly, but ....
		 *
		 * Mark socket as temporary until we're
		 * committed to keeping it.  The code at
		 * ``drop'' and ``dropwithreset'' check the
		 * flag dropsocket to see if the temporary
		 * socket created here should be discarded.
		 * We mark the socket as discardable until
		 * we're committed to it below in TCPS_LISTEN.
		 */
		dropsocket++;
		inp = (struct inpcb *)so->so_pcb;
		inp->inp_laddr = ti->ti_dst;
		inp->inp_lport = ti->ti_dport;
		inp->inp_options = ip_srcroute();
		tp = intotcpcb(inp);
		tp->t_state = TCPS_LISTEN;
	}

	/*
	 * Segment received on connection.
	 * Reset idle time and keep-alive timer.
	 */
	tp->t_idle = 0;
	tp->t_timer[TCPT_KEEP] = TCPTV_KEEP;

	/*
	 * Process options if not in LISTEN state,
	 * else do it below (after getting remote address).
	 */
	if (om && tp->t_state != TCPS_LISTEN) {
		tcp_dooptions(tp, om, ti);
		om = 0;
	}

	/*
	 * Calculate amount of space in receive window,
	 * and then do TCP input processing.
	 * Receive window is amount of space in rcv queue,
	 * but not less than advertised window.
	 */
	{ int win;
	
	win = sbspace(&so->so_rcv);
	if (win < 0)
		win = 0;
	tp->rcv_wnd = MAX(win, (int)(tp->rcv_adv - tp->rcv_nxt));
	}

	switch (tp->t_state) {

	/*
	 * If the state is LISTEN then ignore segment if it contains an RST.
	 * If the segment contains an ACK then it is bad and send a RST.
	 * If it does not contain a SYN then it is not interesting; drop it.
	 * Don't bother responding if the destination was a broadcast.
	 * Otherwise initialize tp->rcv_nxt, and tp->irs, select an initial
	 * tp->iss, and send a segment:
	 *     <SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
	 * Also initialize tp->snd_nxt to tp->iss+1 and tp->snd_una to tp->iss.
	 * Fill in remote peer address fields if not previously specified.
	 * Enter SYN_RECEIVED state, and process any other fields of this
	 * segment in this state.
	 */
	case TCPS_LISTEN: {
		struct mbuf *am;
		register struct sockaddr_in *sin;

		if (tiflags & TH_RST){
			goto drop;
		}
		if (tiflags & TH_ACK){
			goto dropwithreset;
		}
		if ((tiflags & TH_SYN) == 0){
			goto drop;}
		if (in_broadcast(ti->ti_dst)){
			goto drop;
		}
		am = m_get(M_DONTWAIT, MT_SONAME);
		if (am == NULL){
			goto drop;
		}
		am->m_len = sizeof (struct sockaddr_in);
		sin = mtod(am, struct sockaddr_in *);
		sin->sin_family = AF_INET;
		sin->sin_addr = ti->ti_src;
		sin->sin_port = ti->ti_sport;
		laddr = inp->inp_laddr;
		if (inp->inp_laddr.s_addr == INADDR_ANY)
			inp->inp_laddr = ti->ti_dst;
		/* Call in_pcbconnect() with smp lock set */
		/* 4.13.89.us Following if/else is brought
		 * to you courtesy of the protocol timeout
		 * timers.
		 */
		if (!owner){
			if (in_pcbconnect(inp, am)) {
				inp->inp_laddr = laddr;
				(void) m_free(am);
				goto drop;
			}
		}else{
			so->ref = 9;
		  	smp_unlock(&so->lk_socket);
		  	smp_lock(&lk_tcb, LK_RETRY);
			smp_lock(&so->lk_socket, LK_RETRY);
			so->ref = 0;
			if (in_pcbconnect(inp, am)) {
				smp_unlock(&lk_tcb);
				inp->inp_laddr = laddr;
				(void) m_free(am);
				goto drop;
			}
			smp_unlock(&lk_tcb);
		}
		(void) m_free(am);
		tp->t_template = tcp_template(tp);
		if (tp->t_template == 0) {
			tp = tcp_drop(tp, ENOBUFS);
			dropsocket = 0;
			if ( smp && smp_owner(&so->lk_socket) == 0){
				dropsocket = 0;
				goto drop;
			}
			goto drop;
		}
		if (om) {
			tcp_dooptions(tp, om, ti);
			om = 0;
		}
		smp_lock(&lk_tcpiss, LK_RETRY);
		if (iss)
			tp->iss = iss;
		else
			tp->iss = tcp_iss;
		tcp_iss += TCP_ISSINCR/2;
		smp_unlock(&lk_tcpiss);
		tp->irs = ti->ti_seq;
		tcp_sendseqinit(tp);
		tcp_rcvseqinit(tp);
		tp->t_flags |= TF_ACKNOW;
#ifdef XTI
		if (tp->t_acceptmode) {
		  tp->t_flags &= ~(TF_ACKNOW);
		  soisconnected(so);
		}

		/* bump T_LISTEN count in socket struct field
		 * so->so_xticb.xti_evtarray when so->so_xticb.xti_epvalid and
		 * *->evtenabled are on.
		 */
		
		if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
		  
		  /* load tcp options to be read by *_CONOPT */

		  tp->xti_inbound_qos.precedence = xti_precedence;
		  tp->xti_inbound_qos.timeout = tp->xti_neg_qos.timeout;
		  tp->xti_inbound_qos.max_seg_size = tp->t_maxseg;
		  tp->xti_inbound_qos.secopt = xti_secopt;

		  if (so->so_xticb.xti_evtenabled) {
		    so->so_xticb.xti_evtarray[XTI_EVT_T_LISTEN]++;
		    PRINTXTID(1, ("T_LISTEN (#1)    \n"));
		  }
		}
#endif XTI
		tp->t_state = TCPS_SYN_RECEIVED;
		tp->t_timer[TCPT_KEEP] = TCPTV_KEEP;
		dropsocket = 0;		/* committed to socket */
		TCPSTAT(tcps_accepts++);
		goto trimthenstep6;
		}

	/*
	 * If the state is SYN_SENT:
	 *	if seg contains an ACK, but not for our SYN, drop the input.
	 *	if seg contains a RST, then drop the connection.
	 *	if seg does not contain SYN, then drop it.
	 * Otherwise this is an acceptable SYN segment
	 *	initialize tp->rcv_nxt and tp->irs
	 *	if seg contains ack then advance tp->snd_una
	 *	if SYN has been acked change to ESTABLISHED else SYN_RCVD state
	 *	arrange for segment to be acked (eventually)
	 *	continue processing rest of data/controls, beginning with URG
	 */
	case TCPS_SYN_SENT:
		if ((tiflags & TH_ACK) &&
		    (SEQ_LEQ(ti->ti_ack, tp->iss) ||
		     SEQ_GT(ti->ti_ack, tp->snd_max))) {
			goto dropwithreset;
		}
		if (tiflags & TH_RST) {
			if (tiflags & TH_ACK) {
				freeondrop = 0;
#ifdef XTI
	      /* set T_DISCONNECT bit on in socket struct field
	       * so->so_xticb.xti_evtarray when so->so_xticb.xti_epvalid and
	       * *->evtenabled are on.
	       */
				
	      if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
		if (so->so_xticb.xti_evtenabled) {
		  so->so_xticb.xti_evtarray[XTI_EVT_T_DISCONNECT]++;
		  PRINTXTID(1, ("T_DISCONNECT (#1) \n"));
		}
	      }
#endif XTI
				tp = tcp_drop(tp, ECONNREFUSED);
				if (smp &&  smp_owner(&so->lk_socket) == 0){
				   dropsocket = 0;
				   goto drop;
				}
			}
			goto drop;
		}
		if ((tiflags & TH_SYN) == 0){
			goto drop;
		}
		if (tiflags & TH_ACK) {
		tp->snd_una = ti->ti_ack;
		if (SEQ_LT(tp->snd_nxt, tp->snd_una))
			tp->snd_nxt = tp->snd_una;
		}
		tp->t_timer[TCPT_REXMT] = 0;
		tp->irs = ti->ti_seq;
		tcp_rcvseqinit(tp);
		tp->t_flags |= TF_ACKNOW;
		if (tiflags & TH_ACK && SEQ_GT(tp->snd_una, tp->iss)) {
			TCPSTAT(tcps_connects++);
			soisconnected(so);
			tp->t_state = TCPS_ESTABLISHED;
			tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
#ifdef XTI
			/* set T_CONNECT bit on in socket struct field
			 * so->so_xticb.xti_evtarray when so->so_xticb.xti_epvalid and
			 * *->xti_evtenabled are on.
			 */
			
			if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
			  if (so->so_xticb.xti_evtenabled) {
			    so->so_xticb.xti_evtarray[XTI_EVT_T_CONNECT]++; 
			    PRINTXTID(1, ("T_CONNECT (#1)   \n"));
			  }
			  tp->xti_inbound_qos.precedence = xti_precedence;
			  tp->xti_inbound_qos.timeout = tp->xti_neg_qos.timeout;
			  tp->xti_inbound_qos.max_seg_size = tp->t_maxseg;
			  tp->xti_inbound_qos.secopt = xti_secopt;
			}
#endif XTI
			(void) tcp_reass(tp, (struct tcpiphdr *)0);
			/*
			 * if we didn't have to retransmit the SYN,
			 * use its rtt as our initial srtt & rtt var.
			 */
			if (tp->t_rtt) {
				tp->t_srtt = tp->t_rtt << 3;
				tp->t_rttvar = tp->t_rtt << 1;
				TCPT_RANGESET(tp->t_rxtcur, 
				    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
				    TCPTV_MIN, TCPTV_REXMTMAX);
				tp->t_rtt = 0;
			}
#ifdef XTI
		      } else {
			tp->t_state = TCPS_SYN_RECEIVED;

			/* set T_LISTEN bit on in socket struct field
			 * so->so_xticb.xti_evtarray when so->so_xticb.xti_epvalid and
			 * *->xti_evtenabled are on.
			 */
			
			if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
			  /* load tcp options to be read by *_CONOPT */
			  
			  tp->xti_inbound_qos.precedence = xti_precedence;
			  tp->xti_inbound_qos.timeout = tp->xti_neg_qos.timeout;
			  tp->xti_inbound_qos.max_seg_size = tp->t_maxseg;
			  tp->xti_inbound_qos.secopt = xti_secopt;
			  
			  if (so->so_xticb.xti_evtenabled) {
			    so->so_xticb.xti_evtarray[XTI_EVT_T_LISTEN]++; 
			    PRINTXTID(1, ("T_LISTEN (#2) \n"));
			  }
			}
		      }
#else
		} else
			tp->t_state = TCPS_SYN_RECEIVED;

#endif XTI

trimthenstep6:
		/*
		 * Advance ti->ti_seq to correspond to first data byte.
		 * If data, trim to stay within window,
		 * dropping FIN if necessary.
		 */
		ti->ti_seq++;
		if (ti->ti_len > tp->rcv_wnd) {
			todrop = ti->ti_len - tp->rcv_wnd;
			m_adj(m, -todrop);
			ti->ti_len = tp->rcv_wnd;
			tiflags &= ~TH_FIN;
			TCPSTAT(tcps_rcvpackafterwin++);
			TCPSTAT(tcps_rcvbyteafterwin += todrop);
		}
		tp->snd_wl1 = ti->ti_seq - 1;
		tp->rcv_up = ti->ti_seq;
		goto step6;
	}

	/*
	 * States other than LISTEN or SYN_SENT.
	 * First check that at least some bytes of segment are within 
	 * receive window.  If segment begins before rcv_nxt,
	 * drop leading data (and SYN); if nothing left, just ack.
	 */
		todrop = tp->rcv_nxt - ti->ti_seq;
		if (todrop > 0) {
			if (tiflags & TH_SYN) {
				tiflags &= ~TH_SYN;
				ti->ti_seq++;
				if (ti->ti_urp > 1) 
					ti->ti_urp--;
				else
					tiflags &= ~TH_URG;
				todrop--;
			}
			if (todrop > ti->ti_len ||
			todrop == ti->ti_len && (tiflags&TH_FIN) == 0) {
/* #ifdef TCP_COMPAT_42 */
				/*
				 * Don't toss RST in response to 4.2-style keepalive.
				 */
				if (ti->ti_seq == tp->rcv_nxt - 1 && tiflags & TH_RST){
					goto do_rst;}
/* #endif */
				TCPSTAT(tcps_rcvduppack++);
				TCPSTAT(tcps_rcvdupbyte += ti->ti_len);
				todrop = ti->ti_len;
				tiflags &= ~TH_FIN;
				tp->t_flags |= TF_ACKNOW;
			} else {
				TCPSTAT(tcps_rcvpartduppack++);
				TCPSTAT(tcps_rcvpartdupbyte += todrop);
			}
			m_adj(m, todrop);
			ti->ti_seq += todrop;
			ti->ti_len -= todrop;
			if (ti->ti_urp > todrop)
				ti->ti_urp -= todrop;
			else {
				tiflags &= ~TH_URG;
				ti->ti_urp = 0;
			}
		}
	/*
	 * If new data is received on a connection after the
	 * user processes are gone, then RST the other end.
	 */
	if ((so->so_state & SS_NOFDREF) &&
	    tp->t_state > TCPS_CLOSE_WAIT && ti->ti_len) {
		freeondrop = 0;
		TCPSTAT(tcps_rcvafterclose++);
		tp = tcp_close(tp);
		if ( smp && smp_owner(&so->lk_socket) == 0){
			dropsocket = 0;
			/* The socket has been freed by tcp_close()
			 * The above smp_owner check should be removed
			 * The following line will prevent the machine
			 * from panicing.
			 */
			so = NULL;
			goto dropwithreset;
		}
		goto dropwithreset;
	}

	if (tp->rcv_wnd == 0) {
		/*
		 * If window is closed can only take segments at
		 * window edge, and have to drop data and PUSH from
		 * incoming segments.
		 */
		if (tp->rcv_nxt != ti->ti_seq) {
			TCPSTAT(tcps_rcvpackafterwin++);
			TCPSTAT(tcps_rcvbyteafterwin += ti->ti_len);
			goto dropafterack;
		}
		if (ti->ti_len > 0) {
			if (ti->ti_len == 1) 
				TCPSTAT(tcps_rcvwinprobe++);
			else {
				TCPSTAT(tcps_rcvpackafterwin++);
				TCPSTAT(tcps_rcvbyteafterwin += ti->ti_len);
			}
			m_adj(m, ti->ti_len);
			ti->ti_len = 0;
			tiflags &= ~(TH_PUSH|TH_FIN);
		}
	} else {
		/*
		 * If segment ends after window, drop trailing data
		 * (and PUSH and FIN); if nothing left, just ACK.
		 */
		todrop = (ti->ti_seq+ti->ti_len) - (tp->rcv_nxt+tp->rcv_wnd);
		if (todrop > 0) {
			if (todrop >= ti->ti_len) {
				/*
				 * If a new connection request is received
				 * while in TIME_WAIT, drop the old connection
				 * and start over if the sequence numbers
				 * are above the previous ones.
				 */
				if (tiflags & TH_SYN &&
				    tp->t_state == TCPS_TIME_WAIT &&
				    SEQ_GT(ti->ti_seq, tp->rcv_nxt)) {
					iss = tp->rcv_nxt + TCP_ISSINCR;
					(void) tcp_close(tp);
					/* sofree may not really free the
					 * socket and will return with the old
					 * socket locked.
					 */
					if (smp && smp_owner(&so->lk_socket))
						smp_unlock(&so->lk_socket);
					goto findpcb;
				}
				if (todrop == 1) 
					TCPSTAT(tcps_rcvwinprobe++);
				else {
					TCPSTAT(tcps_rcvpackafterwin++);
					TCPSTAT(tcps_rcvbyteafterwin += ti->ti_len);
				}
				goto dropafterack;
			}
			TCPSTAT(tcps_rcvpackafterwin++);
			TCPSTAT(tcps_rcvbyteafterwin += todrop);
			m_adj(m, -todrop);
			ti->ti_len -= todrop;
			tiflags &= ~(TH_PUSH|TH_FIN);
		}
	}

/* #ifdef TCP_COMPAT_42 */
do_rst:
/* #endif */
	/*
	 * If the RST bit is set examine the state:
	 *    SYN_RECEIVED STATE:
	 *	If passive open, return to LISTEN state.
	 *	If active open, inform user that connection was refused.
	 *    ESTABLISHED, FIN_WAIT_1, FIN_WAIT2, CLOSE_WAIT STATES:
	 *	Inform user that connection was reset, and close tcb.
	 *    CLOSING, LAST_ACK, TIME_WAIT STATES
	 *	Close the tcb.
	 */
	if (tiflags&TH_RST) switch (tp->t_state) {

	case TCPS_SYN_RECEIVED:
#ifdef XTI
		/* set T_DISCONNECT bit on in socket struct field
		 * so->so_xticb.xti_evtarray when so->so_xticb.xti_epvalid and
		 * *->xti_evtenabled are on.
		 */
	
		if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
		  if (so->so_xticb.xti_evtenabled) {
		    so->so_xticb.xti_evtarray[XTI_EVT_T_DISCONNECT]++; 
		    PRINTXTID(1, ("T_DISCONNECT (#2) \n"));
		  }
		}

#endif XTI
		freeondrop = 0;
		tp = tcp_drop(tp, ECONNREFUSED);
		if (smp && smp_owner(&so->lk_socket) == 0){
			dropsocket = 0;
			goto drop;
		}
		goto drop;

	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
#ifdef XTI
		/* set T_DISCONNECT bit on in socket struct field
		 * so->so_xticb.xti_evtarray when so->so_xticb.xti_epvalid and
		 * *->xti_evtenabled are on.
		 */
		
		if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
		  if (so->so_xticb.xti_evtenabled) {
		    so->so_xticb.xti_evtarray[XTI_EVT_T_DISCONNECT]++; 
		    PRINTXTID(1, ("T_DISCONNECT (#3) \n"));
		  }
		}
#endif XTI
		freeondrop = 0;
		tp = tcp_drop(tp, ECONNRESET);
		if (smp && smp_owner(&so->lk_socket) == 0){
			dropsocket = 0;
			goto drop;
		}
		goto drop;

	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:
#ifdef XTI

		/* set T_DISCONNECT bit on in socket struct field
		 * so->so_xticb.xti_evtarray when so->so_xticb.xti_epvalid and
		 * *->xti_evtenabled are on.
		 */
	
		if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
		  if (so->so_xticb.xti_evtenabled) {
		    so->so_xticb.xti_evtarray[XTI_EVT_T_DISCONNECT]++; 
		    PRINTXTID(1, ("T_DISCONNECT (#4) \n"));
		  }
		}
#endif XTI
		freeondrop = 0;
		tp = tcp_close(tp);
#ifdef TCPLOOPBACK
	case TCPS_CLOSED:
#endif
		if (smp && smp_owner(&so->lk_socket) == 0){
			dropsocket = 0;
			goto drop;
		}
		goto drop;
	}

	/*
	 * If a SYN is in the window, then this is an
	 * error and we send an RST and drop the connection.
	 */
	/* tcp_drop cannot be called before dropwithreset without
	 * guaranteeing a good socket and tp.  Duplicate all 
	 * dropwithreset here, but change where tcp_drop() is called.
	 */
	if (tiflags & TH_SYN) {
		if ((tiflags & TH_RST) || in_broadcast(ti->ti_dst)){
			goto drop;
		}
		if (tiflags & TH_ACK){
			tcp_respond(tp,ti,(tcp_seq)0,ti->ti_ack, TH_RST, so);
		}
		else {
			if (tiflags & TH_SYN)
				ti->ti_len++;
			tcp_respond(tp, ti, ti->ti_seq+ti->ti_len, (tcp_seq)0,
			    TH_RST|TH_ACK, so);
		}
		tp = tcp_drop(tp, ECONNRESET);
		if (om) {
			(void) m_free(om);
		}
		if (dropsocket) {
			if (smp && smp_owner(&so->lk_socket) == 1)
				(void) soabort(so);
			if (smp && smp_owner(&so->lk_socket))
				smp_unlock(&so->lk_socket);
			if ( !smp )
				(void) soabort(so);
		} else {
			if (smp && smp_owner(&so->lk_socket))
				smp_unlock(&so->lk_socket);
			}
		return;
	}

	/*
	 * If the ACK bit is off we drop the segment and return.
	 */
	if ((tiflags & TH_ACK) == 0){
		goto drop;
	}
	
	/*
	 * Ack processing.
	 */
	switch (tp->t_state) {

	/*
	 * In SYN_RECEIVED state if the ack ACKs our SYN then enter
	 * ESTABLISHED state and continue processing, otherwise
	 * send an RST.
	 */
	case TCPS_SYN_RECEIVED:
		if (SEQ_GT(tp->snd_una, ti->ti_ack) ||
		    SEQ_GT(ti->ti_ack, tp->snd_max)){
			goto dropwithreset;
		}
		TCPSTAT(tcps_connects++);
#ifdef XTI
		if (!(tp->t_acceptmode))
#endif XTI
		soisconnected(so);
		tp->t_state = TCPS_ESTABLISHED;
		tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
		(void) tcp_reass(tp, (struct tcpiphdr *)0);
		tp->snd_wl1 = ti->ti_seq - 1;
		/* fall into ... */

	/*
	 * In ESTABLISHED state: drop duplicate ACKs; ACK out of range
	 * ACKs.  If the ack is in the range
	 *	tp->snd_una < ti->ti_ack <= tp->snd_max
	 * then advance tp->snd_una to ti->ti_ack and drop
	 * data from the retransmission queue.  If this ACK reflects
	 * more up to date window information we update our window information.
	 */
	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:

		if (SEQ_LEQ(ti->ti_ack, tp->snd_una)) {
			if (ti->ti_len == 0 && ti->ti_win == tp->snd_wnd) {
				TCPSTAT(tcps_rcvdupack++);
				/*
				 * If we have outstanding data (not a
				 * window probe), this is a completely
				 * duplicate ack (ie, window info didn't
				 * change), the ack is the biggest we've
				 * seen and we've seen exactly our rexmt
				 * threshhold of them, assume a packet
				 * has been dropped and retransmit it.
				 * Kludge snd_nxt & the congestion
				 * window so we send only this one
				 * packet.  If this packet fills the
				 * only hole in the receiver's seq.
				 * space, the next real ack will fully
				 * open our window.  This means we
				 * have to do the usual slow-start to
				 * not overwhelm an intermediate gateway
				 * with a burst of packets.  Leave
				 * here with the congestion window set
				 * to allow 2 packets on the next real
				 * ack and the exp-to-linear thresh
				 * set for half the current window
				 * size (since we know we're losing at
				 * the current window size).
				 */
				if (tp->t_timer[TCPT_REXMT] == 0 ||
				    ti->ti_ack != tp->snd_una)
					tp->t_dupacks = 0;
				else if (++tp->t_dupacks == tcprexmtthresh) {
					tcp_seq onxt = tp->snd_nxt;
					u_int win =
					    MIN(tp->snd_wnd, tp->snd_cwnd) / 2 /
						tp->t_maxseg;

					if (win < 2)
						win = 2;
					tp->snd_ssthresh = win * tp->t_maxseg;

					tp->t_timer[TCPT_REXMT] = 0;
					tp->t_rtt = 0;
					tp->snd_nxt = ti->ti_ack;
					tp->snd_cwnd = tp->t_maxseg;
					(void) tcp_output(tp);

					if (SEQ_GT(onxt, tp->snd_nxt))
						tp->snd_nxt = onxt;
					goto drop;
				}
			} else
				tp->t_dupacks = 0;
			break;
		}
		tp->t_dupacks = 0;
		if (SEQ_GT(ti->ti_ack, tp->snd_max)) {
			TCPSTAT(tcps_rcvacktoomuch++);
			goto dropafterack;
		}
		acked = ti->ti_ack - tp->snd_una;
		TCPSTAT(tcps_rcvackpack++);
		TCPSTAT(tcps_rcvackbyte += acked);

		/*
		 * If transmit timer is running and timed sequence
		 * number was acked, update smoothed round trip time.
		 * Since we now have an rtt measurement, cancel the
		 * timer backoff (cf., Phil Karn's retransmit alg.).
		 * Recompute the initial retransmit timer.
		 */
		if (tp->t_rtt && SEQ_GT(ti->ti_ack, tp->t_rtseq)) {
			TCPSTAT(tcps_rttupdated++);
			if (tp->t_srtt != 0) {
				register short delta;

				/*
				 * srtt is stored as fixed point with 3 bits
				 * after the binary point (i.e., scaled by 8).
				 * The following magic is equivalent
				 * to the smoothing algorithm in rfc793
				 * with an alpha of .875
				 * (srtt = rtt/8 + srtt*7/8 in fixed point).
				 * Adjust t_rtt to origin 0.
				 */
				tp->t_rtt--;
				delta = tp->t_rtt - (tp->t_srtt >> 3);
				if ((tp->t_srtt += delta) <= 0)
					tp->t_srtt = 1;
				/*
				 * We accumulate a smoothed rtt variance
				 * (actually, a smoothed mean difference),
				 * then set the retransmit timer to smoothed
				 * rtt + 2 times the smoothed variance.
				 * rttvar is stored as fixed point
				 * with 2 bits after the binary point
				 * (scaled by 4).  The following is equivalent
				 * to rfc793 smoothing with an alpha of .75
				 * (rttvar = rttvar*3/4 + |delta| / 4).
				 * This replaces rfc793's wired-in beta.
				 */
				if (delta < 0)
					delta = -delta;
				delta -= (tp->t_rttvar >> 2);
				if ((tp->t_rttvar += delta) <= 0)
					tp->t_rttvar = 1;
			} else {
				/* 
				 * No rtt measurement yet - use the
				 * unsmoothed rtt.  Set the variance
				 * to half the rtt (so our first
				 * retransmit happens at 2*rtt)
				 */
				tp->t_srtt = tp->t_rtt << 3;
				tp->t_rttvar = tp->t_rtt << 1;
			}
			tp->t_rtt = 0;
			tp->t_rxtshift = 0;
			TCPT_RANGESET(tp->t_rxtcur, 
			    ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1,
			    TCPTV_MIN, TCPTV_REXMTMAX);
		}

		/*
		 * If all outstanding data is acked, stop retransmit
		 * timer and remember to restart (more output or persist).
		 * If there is more data to be acked, restart retransmit
		 * timer, using current (possibly backed-off) value.
		 */
		if (ti->ti_ack == tp->snd_max) {
			tp->t_timer[TCPT_REXMT] = 0;
			tp->t_rxtshift = 0; /* reset to initial value */
			needoutput = 1;
		} else if (tp->t_timer[TCPT_PERSIST] == 0)
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
		/*
		 * When new data is acked, open the congestion window.
		 * If the window gives us less than ssthresh packets
		 * in flight, open exponentially (maxseg per packet).
		 * Otherwise open linearly (maxseg per window,
		 * or maxseg^2 / cwnd per packet).
		 */
		{
		u_int incr = tp->t_maxseg;

		if (tp->snd_cwnd > tp->snd_ssthresh)
			incr = MAX(incr * incr / tp->snd_cwnd, 1);

		tp->snd_cwnd = MIN(tp->snd_cwnd + incr, 65535); /* XXX */
		}
		if (acked > so->so_snd.sb_cc) {
			tp->snd_wnd -= so->so_snd.sb_cc;
			sbdrop(&so->so_snd, (int)so->so_snd.sb_cc);
			ourfinisacked = 1;
#ifdef XTI
			if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
			  if (so->so_xticb.xti_evtenabled) {
			    if (so->so_xticb.xti_blocked) { /* flow controlled EWOULDBLOCK */
			      so->so_xticb.xti_blocked = 0; /* unblocked */
			      so->so_xticb.xti_evtarray[XTI_EVT_T_GODATA]++; 
			      so->so_xticb.xti_evtarray[XTI_EVT_T_GOEXDATA]++; 
			      PRINTXTID(1, ("T_GODATA (#1)    \n"));
			      PRINTXTID(1, ("T_GOEXDATA (#1)    \n"));
			    }
			  }
			}	
#endif XTI
		} else {
			sbdrop(&so->so_snd, acked);
			tp->snd_wnd -= acked;
			ourfinisacked = 0;
#ifdef XTI
			if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
			  if (so->so_xticb.xti_evtenabled) {
			    if (so->so_xticb.xti_blocked) { /* flow controlled EWOULDBLOCK */
			      so->so_xticb.xti_blocked = 0; /* unblocked */
			      so->so_xticb.xti_evtarray[XTI_EVT_T_GODATA]++; 
			      so->so_xticb.xti_evtarray[XTI_EVT_T_GOEXDATA]++; 
			      PRINTXTID(1, ("T_GODATA (#2)    \n"));
			      PRINTXTID(1, ("T_GOEXDATA (#2)    \n"));
			    }
			  }
			}	
#endif XTI
		}
		if ((so->so_snd.sb_flags & SB_WAIT) || so->so_snd.sb_sel)
			sowwakeup(so);
		tp->snd_una = ti->ti_ack;
		if (SEQ_LT(tp->snd_nxt, tp->snd_una))
			tp->snd_nxt = tp->snd_una;

		switch (tp->t_state) {

		/*
		 * In FIN_WAIT_1 STATE in addition to the processing
		 * for the ESTABLISHED state if our FIN is now acknowledged
		 * then enter FIN_WAIT_2.
		 */
		case TCPS_FIN_WAIT_1:
			if (ourfinisacked) {
				/*
				 * If we can't receive any more
				 * data, then closing user can proceed.
				 * Starting the timer is contrary to the
				 * specification, but if we don't get a FIN
				 * we'll hang forever.
				 */
				if (so->so_state & SS_CANTRCVMORE) {
					soisdisconnected(so);
					tp->t_timer[TCPT_2MSL] = TCPTV_MAXIDLE;
				}
				tp->t_state = TCPS_FIN_WAIT_2;
			}
			break;

	 	/*
		 * In CLOSING STATE in addition to the processing for
		 * the ESTABLISHED state if the ACK acknowledges our FIN
		 * then enter the TIME-WAIT state, otherwise ignore
		 * the segment.
		 */
		case TCPS_CLOSING:
			if (ourfinisacked) {
				tp->t_state = TCPS_TIME_WAIT;
				tcp_canceltimers(tp);
				tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
				soisdisconnected(so);
			}
			break;

		/*
		 * In LAST_ACK, we may still be waiting for data to drain
		 * and/or to be acked, as well as for the ack of our FIN.
		 * If our FIN is now acknowledged, delete the TCB,
		 * enter the closed state and return.
		 */
		case TCPS_LAST_ACK:
			if (ourfinisacked) {
				freeondrop = 0;
				tp = tcp_close(tp);
				if ( smp && smp_owner(&so->lk_socket) == 0){
				    dropsocket = 0;
				    goto drop;
				}
				goto drop;
			}
			break;

		/*
		 * In TIME_WAIT state the only thing that should arrive
		 * is a retransmission of the remote FIN.  Acknowledge
		 * it and restart the finack timer.
		 */
		case TCPS_TIME_WAIT:
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			goto dropafterack;
		}
	}

step6:
	/*
	 * Update window information.
	 * Don't look at window if no ACK: TAC's send garbage on first SYN.
	 */
	if ((tiflags & TH_ACK) &&
            (SEQ_LT(tp->snd_wl1, ti->ti_seq) || tp->snd_wl1 == ti->ti_seq &&
	    (SEQ_LT(tp->snd_wl2, ti->ti_ack) ||
	     tp->snd_wl2 == ti->ti_ack && ti->ti_win > tp->snd_wnd))) {
		/* keep track of pure window updates */
		if (ti->ti_len == 0 &&
		    tp->snd_wl2 == ti->ti_ack && ti->ti_win > tp->snd_wnd)
			TCPSTAT(tcps_rcvwinupd++);
		tp->snd_wnd = ti->ti_win;
		tp->snd_wl1 = ti->ti_seq;
		tp->snd_wl2 = ti->ti_ack;
		if (tp->snd_wnd > tp->max_sndwnd)
			tp->max_sndwnd = tp->snd_wnd;
		needoutput = 1;
	}

	/*
	 * Process segments with URG.
	 */
	if ((tiflags & TH_URG) && ti->ti_urp &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		/*
		 * This is a kludge, but if we receive and accept
		 * random urgent pointers, we'll crash in
		 * soreceive.  It's hard to imagine someone
		 * actually wanting to send this much urgent data.
		 */
		if (ti->ti_urp + so->so_rcv.sb_cc > SB_MAX) {
			ti->ti_urp = 0;			/* XXX */
			tiflags &= ~TH_URG;		/* XXX */
			goto dodata;			/* XXX */
		}
		/*
		 * If this segment advances the known urgent pointer,
		 * then mark the data stream.  This should not happen
		 * in CLOSE_WAIT, CLOSING, LAST_ACK or TIME_WAIT STATES since
		 * a FIN has been received from the remote side. 
		 * In these states we ignore the URG.
		 *
		 * According to RFC961 (Assigned Protocols),
		 * the urgent pointer points to the last octet
		 * of urgent data.  We continue, however,
		 * to consider it to indicate the first octet
		 * of data past the urgent section
		 * as the original spec states.
		 */
		if (SEQ_GT(ti->ti_seq+ti->ti_urp, tp->rcv_up)) {
			tp->rcv_up = ti->ti_seq + ti->ti_urp;
			so->so_oobmark = so->so_rcv.sb_cc +
			    (tp->rcv_up - tp->rcv_nxt) - 1;

			if (so->so_oobmark == 0)
				so->so_state |= SS_RCVATMARK;
			sohasoutofband(so);
			tp->t_oobflags &= ~(TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		      }
		/*
		 * Remove out of band data so doesn't get presented to user.
		 * This can happen independent of advancing the URG pointer,
		 * but if two URG's are pending at once, some out-of-band
		 * data may creep in... ick.
		 */
		if (ti->ti_urp <= ti->ti_len &&
#ifdef XTI
		    (so->so_options & SO_OOBINLINE) == 0) {
		  PRINTXTID(32, ("Calling tcp_pulloobxti\n"));
		  tcp_pulloobxti(so, ti);
		} else
		  /*
		   * Because they don't present oob data to user
		   * we must update our event structures.
		   */
		  {
		    /* For now do not touch event info
		    sbdrop(&so->so_exrcv, so->so_exrcv.sb_cc);
		    so->so_xticb.xti_evtarray[XTI_EVT_T_EXDATA] = 0;
		    */
		  }
#else
			(so->so_options & SO_OOBINLINE) == 0)
			tcp_pulloutofband(so, ti);
#endif XTI
	} else
		/*
		 * If no out of band data is expected,
		 * pull receive urgent pointer along
		 * with the receive window.
		 */
		if (SEQ_GT(tp->rcv_nxt, tp->rcv_up))
			tp->rcv_up = tp->rcv_nxt;

dodata:							/* XXX */

	/*
	 * Process the segment text, merging it into the TCP sequencing queue,
	 * and arranging for acknowledgment of receipt if necessary.
	 * This process logically involves adjusting tp->rcv_wnd as data
	 * is presented to the user (this happens in tcp_usrreq.c,
	 * case PRU_RCVD).  If a FIN has already been received on this
	 * connection then we just ignore the text.
	 */
	if ((ti->ti_len || (tiflags&TH_FIN)) &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		TCP_REASS(tp, ti, m, so, tiflags);
		if (tcpnodelack == 0)
			tp->t_flags |= TF_DELACK;
		else
			tp->t_flags |= TF_ACKNOW;
		/*
		 * Note the amount of data that peer has sent into
		 * our window, in order to estimate the sender's
		 * buffer size.
		 */
		len = so->so_rcv.sb_hiwat - (tp->rcv_adv - tp->rcv_nxt);
		if (len > tp->max_rcvd)
			tp->max_rcvd = len;
	} else {
		m_freem(m);
		tiflags &= ~TH_FIN;
	}

	/*
	 * If FIN is received ACK the FIN and let the user know
	 * that the connection is closing.
	 */
	if ((tiflags & TH_FIN) || 

	/* 1/91.us.  According to rfc793, the normal tcp state transitions
	 * on close are:
	 * (side1) ESTABLISHED -> FIN_WAIT_1 -> FIN_WAIT_2 -> TIME_WAIT
	 * (side2) ESTABLISHED -> CLOSE_WAIT -> LAST_ACK
	 * Side2, when in CLOSE_WAIT, is supposed to send a message
	 * to side1 to cause side1 to transition from FIN_WAIT_2 to
	 * TIME_WAIT.  This is not implemented here.  However,
	 * the way rfc793  reads, it seems optional that side2 
	 * should send a message to force the state transition from 
	 * FIN_WAIT_2 to TIME_WAIT.  The problem is that without
	 * the FIN_WAIT_2 to TIME_WAIT transition, the system can
	 * crash in LOOPBACK MODE with tcp_input() and tcp_output()
	 * thrashing until they overwrite the stack.  Side1
	 * will only transition from FIN_WAIT_2 to TIME_WAIT if it has 
	 * received a FIN, as you can see  in the case statement
	 * below.  To avoid the thrashing, we force a transition
	 * from FIN_WAIT_2 to TIME_WAIT for loopback, even when we have
	 * not received FIN on side1.
	 */

	((tp->t_state==TCPS_FIN_WAIT_2) && 
	(tp->t_template->ti_src.s_addr==tp->t_template->ti_dst.s_addr))){

		/* Only do this conditional if you've received FIN */
	     	if (tiflags & TH_FIN){
		   if (TCPS_HAVERCVDFIN(tp->t_state) == 0) {
			socantrcvmore(so);
			tp->t_flags |= TF_ACKNOW;
			tp->rcv_nxt++;
		   }
	      	}
		switch (tp->t_state) {

	 	/*
		 * In SYN_RECEIVED and ESTABLISHED STATES
		 * enter the CLOSE_WAIT state.
		 */
		case TCPS_SYN_RECEIVED:
		case TCPS_ESTABLISHED:
#ifdef XTI

			/* set T_ORDREL bit on in socket struct field
			 * so->so_xticb.xti_evtarray when so->so_xticb.xti_epvalid and
			 * *->xti_evtenabled are on.
			 */
		  
		         if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
			   if (so->so_xticb.xti_evtenabled) {
			     so->so_xticb.xti_evtarray[XTI_EVT_T_ORDREL]++;
			     PRINTXTID(1, ("T_ORDREL (#1) \n"));
			   }
			 } 
#endif XTI
			tp->t_state = TCPS_CLOSE_WAIT;
			break;

	 	/*
		 * If still in FIN_WAIT_1 STATE FIN has not been acked so
		 * enter the CLOSING state.
		 */
		case TCPS_FIN_WAIT_1:

#ifdef XTI
			 /* set T_ORDREL bit on in socket struct field
			  * so->so_xticb.xti_evtarray when so->so_xticb.xti_epvalid and
			  * *->xti_evtenabled are on.
			  */
			 
			 if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
			   if (so->so_xticb.xti_evtenabled) {
			     so->so_xticb.xti_evtarray[XTI_EVT_T_ORDREL]++;
			     PRINTXTID(1, ("T_ORDREL (#3) \n"));
			   }
			 }
#endif XTI
			tp->t_state = TCPS_CLOSING;
			break;

	 	/*
		 * In FIN_WAIT_2 state enter the TIME_WAIT state,
		 * starting the time-wait timer, turning off the other 
		 * standard timers.
		 */
		case TCPS_FIN_WAIT_2:
			/***
	   		if (tp->t_template->ti_src.s_addr==tp->t_template->ti_dst.s_addr)
				mprintf("tcp_input: loopback and FIN_WAIT_2");
			***/

			tp->t_state = TCPS_TIME_WAIT;
#ifdef XTI
			 /* set T_ORDREL bit on in socket struct field
			  * so->so_xticb.xti_evtarray when so->so_xticb.xti_epvalid and
			  * *->xti_evtenabled are on.
			  */
		  
			 if (so->so_xticb.xti_epvalid) { /* valid xti endpoint */
			   if (so->so_xticb.xti_evtenabled) {
			     so->so_xticb.xti_evtarray[XTI_EVT_T_ORDREL]++;
			     PRINTXTID(1, ("T_ORDREL (#2)  \n"));
			   }
			 }
#endif XTI
			tcp_canceltimers(tp);
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			soisdisconnected(so);
			break;

		/*
		 * In TIME_WAIT state restart the 2 MSL time_wait timer.
		 */
		case TCPS_TIME_WAIT:
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			break;
		}
	}

	if (so->so_options & SO_DEBUG)
		tcp_trace(TA_INPUT, ostate, tp, &tcp_saveti, 0);

	/*
	 * Return any desired output.
	 */
#ifdef XTI
	if (tp->t_state >= TCPS_ESTABLISHED || (!(tp->t_acceptmode)))
#endif XTI
	if (needoutput || (tp->t_flags & TF_ACKNOW))
		(void) tcp_output(tp);
	smp_unlock(&so->lk_socket);
	return;

dropafterack:
	/*
	 * Generate an ACK dropping incoming segment if it occupies
	 * sequence space, where the ACK reflects our state.
	 */
	if (tiflags & TH_RST){
		goto drop;
	}
	m_freem(m);
	tp->t_flags |= TF_ACKNOW;
	(void) tcp_output(tp);
	smp_unlock(&so->lk_socket);
	return;

dropwithreset:
	if (om) {
		(void) m_free(om);
		om = 0;
	}
	/*
	 * Generate a RST, dropping incoming segment.
	 * Make ACK acceptable to originator of segment.
	 * Don't bother to respond if destination was broadcast.
	 */
	if ((tiflags & TH_RST) || in_broadcast(ti->ti_dst)){
		goto drop;
	}
	if (tiflags & TH_ACK){
		tcp_respond(tp, ti, (tcp_seq)0, ti->ti_ack, TH_RST, so);
	}
	else {
		if (tiflags & TH_SYN)
			ti->ti_len++;
		tcp_respond(tp, ti, ti->ti_seq+ti->ti_len, (tcp_seq)0,
		    TH_RST|TH_ACK, so);
	}
	/* destroy temporarily created socket */
	if (dropsocket) {
		if (smp && smp_owner(&so->lk_socket) == 1)
			(void) soabort(so);
		if (smp && smp_owner(&so->lk_socket))
			smp_unlock(&so->lk_socket);
		if ( !(smp) )
			(void) soabort(so);
	} else {
		if (smp && smp_owner(&so->lk_socket))
			smp_unlock(&so->lk_socket);
		}
	return;

drop:
	if (om){
		(void) m_free(om);
	}
	/*
	 * Drop space held by incoming segment and return.
	 */
	if (tp && (tp->t_inpcb->inp_socket->so_options & SO_DEBUG))
		tcp_trace(TA_DROP, ostate, tp, &tcp_saveti, 0);
	if ( (m != NULL) ){	/* set m = 0 in ip_output's m_free call */
		m_freem(m);
	}

	/* destroy temporarily created socket */
	if (dropsocket) {
		if (smp && smp_owner(&so->lk_socket) == 1)
			(void) soabort(so);
		if (smp && smp_owner(&so->lk_socket))
				smp_unlock(&so->lk_socket);
		if ( !(smp) )
			(void) soabort(so);
	} else {
		if (smp && smp_owner(&so->lk_socket))
			smp_unlock(&so->lk_socket);
	}
	return;
}

tcp_dooptions(tp, om, ti)
	struct tcpcb *tp;
	struct mbuf *om;
	struct tcpiphdr *ti;
{
	register u_char *cp;
	int opt, optlen, cnt;

	cp = mtod(om, u_char *);
	cnt = om->m_len;
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == TCPOPT_EOL)
			break;
		if (opt == TCPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[1];
			if (optlen <= 0)
				break;
		}
		switch (opt) {

		default:
			break;

		case TCPOPT_MAXSEG:
			if (optlen != 4)
				continue;
			if (!(ti->ti_flags & TH_SYN))
				continue;
			tp->t_maxseg = *(u_short *)(cp + 2);
			tp->t_maxseg = ntohs((u_short)tp->t_maxseg);
			tp->t_maxseg = MIN(tp->t_maxseg, tcp_mss(tp));
			break;
		}
	}
	(void) m_free(om);
}

#ifdef XTI
/*
 * Pull out of band octets out of a segment so
 * it doesn't appear in the user's data queue.
 * It is still reflected in the segment length for
 * sequencing purposes.
 */
tcp_pulloobxti(so, ti)
	struct socket *so;
        struct tcpiphdr *ti;
{
	register struct mbuf *m;
	struct mbuf *m0;
	struct mbuf *m1;
	char *cp;
	char save_urg;
	char *save_urg_ptr;
	int cnt = ti->ti_urp - 1;
	struct tcpcb *tp = sototcpcb(so);
	int xti_flag = 0;
	int new_oob_len; /* oob data we are processing */

	if (so->so_xticb.xti_epvalid)
	  xti_flag = 1;
	  
	save_urg_ptr = &save_urg;
	m = dtom(ti);

	/*
	 * Overwrite section - Update event information
	 */

	if (xti_flag) 
	  if (so->so_xticb.xti_evtenabled) {
	    /* write brand new record */
	    so->so_xticb.xti_evtarray[XTI_EVT_T_EXDATA] = 1;
	    PRINTXTID(1, ("T_EXDATA (#1) (brand new - 1 event) cnt=%d\n",
			  cnt));
	  } /* valid xti endpoint and events enabled */


	if (xti_flag) {
	  new_oob_len = cnt + 1;
          new_oob_len = (new_oob_len <= XTI_EXPED_SIZE) ? new_oob_len : XTI_EXPED_SIZE;
	  m0 = m_copy(m, 0, new_oob_len);
	  m0 = m_pullup(m0, new_oob_len);
	} else {
	  while (cnt >= 0) {
	    if (m->m_len > cnt) {
	      char *cp = mtod(m, caddr_t) + cnt;
	      *save_urg_ptr = *cp;
	      new_oob_len = 1;
	      m0 = m_copy(m, 0, new_oob_len);
              bcopy(save_urg_ptr, mtod(m0, caddr_t), new_oob_len);
	      bcopy(cp+1, cp, (unsigned)(m->m_len - cnt - 1));
	      m->m_len--;
	      break;
	    }
	    cnt -= m->m_len;
	    m = m->m_next;
	    if (m == 0) panic("tcp_pulloutofband");
	  }
	}

	if (m0 == 0) panic("tcp_pulloobxti no m0");

	/*
	 * RB: Since a user may be in the process of reading the
	 *     out-of-band data on another CPU, the socket might
	 *     not be locked.  Therefore, we mustn't free the mbuf
	 *     here and let soreceive free the mbuf after it has
	 *     been read.  So we handle the newly arrived out-of-
	 *     band data by copying the data over the existing
	 *     out-of-band data.
	 */
	if (so->so_exrcv.sb_mb == NULL) {
		sbappend(&so->so_exrcv, m0);
  	} else {
		m1 = so->so_exrcv.sb_mb;
		so->so_exrcv.sb_cc -= m1->m_len;
		so->so_exrcv.sb_cc += (new_oob_len);
		m1->m_len = new_oob_len;
		/*
                 * Fix Ron's fix to smp problem.
		 * Reset the offset or we may walk off the end
	         * of the mbuf resulting in an unaligned access
		 * crash
		 */
		m1->m_off = MMINOFF; 
		bcopy(mtod(m0, caddr_t), mtod(m1, caddr_t), new_oob_len);
		m_freem(m0);
	}
	tp->t_oobflags |= TCPOOB_HAVEDATA;

	if (xti_flag) 
	  while (1) {
	    if (m->m_len >= new_oob_len) {
	      m->m_len -= new_oob_len;
	      if (m->m_len != 0 && m->m_off <= MMAXOFF)
		m->m_off += new_oob_len;
	      break;
	    }
	    else {
	      new_oob_len -= m->m_len;
	      m->m_len = 0;
	      m = m->m_next;
	      if (!m) panic("pulloobxti no m->m_next");
	    }
	  }

	return;
}
#else
/*
 * Pull out of band byte out of a segment so
 * it doesn't appear in the user's data queue.
 * It is still reflected in the segment length for
 * sequencing purposes.
 */
tcp_pulloutofband(so, ti)
	struct socket *so;
	struct tcpiphdr *ti;
{
	register struct mbuf *m;
	int cnt = ti->ti_urp - 1;
	
	m = dtom(ti);
	while (cnt >= 0) {
		if (m->m_len > cnt) {
			char *cp = mtod(m, caddr_t) + cnt;
			struct tcpcb *tp = sototcpcb(so);

			tp->t_iobc = *cp;
			tp->t_oobflags |= TCPOOB_HAVEDATA;
			bcopy(cp+1, cp, (unsigned)(m->m_len - cnt - 1));
			m->m_len--;
			return;
		}
		cnt -= m->m_len;
		m = m->m_next;
		if (m == 0)
			break;
	}
	panic("tcp_pulloutofband");
}
#endif XTI
/*
 *  Determine a reasonable value for maxseg size.
 *  If the route is known, use one that can be handled
 *  on the given interface without forcing IP to fragment.
 *  If bigger than an mbuf cluster (NCLBYTES), round down to nearest size
 *  to utilize large mbufs.
 *  If interface pointer is unavailable, or the destination isn't local,
 *  use a conservative size (512 or the default IP max size, but no more
 *  than the mtu of the interface through which we route),
 *  as we can't discover anything about intervening gateways or networks.
 *  We also initialize the congestion/slow start window to be a single
 *  segment if the destination isn't local; this information should
 *  probably all be saved with the routing entry at the transport level.
 *
 *  This is ugly, and doesn't belong at this level, but has to happen somehow.
 */

int tcp_trailers = 0;

tcp_mss(tp)
	register struct tcpcb *tp;
{
	struct route *ro;
	struct ifnet *ifp;
	int mss;
	struct inpcb *inp;

	inp = tp->t_inpcb;
	RTLOCK();
	ro = &inp->inp_route;
	if ((ro->ro_rt == (struct rtentry *)0) ||
	    (ifp = ro->ro_rt->rt_ifp) == (struct ifnet *)0) {
		/* No route yet, so try to acquire one */
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			ro->ro_dst.sa_family = AF_INET;
			((struct sockaddr_in *) &ro->ro_dst)->sin_addr =
				inp->inp_faddr;
			rtalloc(ro);
		}
		if ((ro->ro_rt == 0) || (ifp = ro->ro_rt->rt_ifp) == 0){
			RTUNLOCK();
			return (TCP_MSS);
		}
	}
	RTUNLOCK();

	mss = ifp->if_mtu - sizeof(struct tcpiphdr);

	if((ifp->if_flags & IFF_NOTRAILERS) == 0) {
#if	(NCLBYTES & (NCLBYTES - 1)) == 0
	if (mss > NCLBYTES) {
		mss = NCLBYTES;
	}
#else
	if (mss > NCLBYTES)
		mss = mss / NCLBYTES * NCLBYTES;
#endif
	}
	if (in_localaddr(inp->inp_faddr))
		return (mss);

	mss = MIN(mss, TCP_MSS);
	tp->snd_cwnd = mss;
	return (mss);
}

#ifdef XTI
xti_ip_dooptions(ip,xti_tcp_opt)
        register struct ip *ip;
        struct secoptions *xti_tcp_opt;
{
	register u_char *cp;
	int opt, optlen, cnt;
	int return_stat = 1; /* default to failure  */

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
	    if (optlen <= 0 || optlen > cnt) return(1);
	  }
	  
	  switch (opt) {
	    
	  case IPOPT_SECURITY:
	    {
	      short security;
	      short compartment;
	      short handling;
	      long tcc;
	      int i;
	      
	      bcopy(&cp[IPOPT_OFFSET], &security, sizeof(short));
	      security = ntohs(security);
	      
	      bcopy(&cp[IPOPT_OFFSET + sizeof(short)], &compartment, sizeof(short));
	      compartment = ntohs(compartment);
	      
	      bcopy(&cp[IPOPT_OFFSET + (2*sizeof(short))], &handling, sizeof(short));
	      handling = ntohs(handling);
	      
	      bcopy(&cp[IPOPT_OFFSET + (3*sizeof(short))], &tcc, sizeof(long int));
	      tcc = ntohl(tcc);
	      
	      xti_tcp_opt->security = security;
	      xti_tcp_opt->compartment = compartment;
	      xti_tcp_opt->handling = handling;
	      xti_tcp_opt->tcc = tcc;
	      
	      return_stat = 0; /* success */
	      /*
	       * bcopy(&cp[IPOPT_OFFSET], xti_tcp_opt, optlen - 2);
	       */

	    }
	    break;
	    
	  default:
	    break;

	  };
	}
	return (return_stat);
}

/*
 * Any data in this mbuf?
 */

int mbuf_any_len(m)
        struct mbuf *m;
{
        int tot_len = 0;

	if (m)
	  do {
	    tot_len += m->m_len;
	    if (tot_len > 0)
	      return (tot_len);
	    m = m->m_next;
	  } while (m);
	return(tot_len);
}
#endif XTI

