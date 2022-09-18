#ifndef lint
static char *sccsid = "@(#)tcp_subr.c	4.1    ULTRIX                  7/2/90";
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
 *									*
 *	2-Jan_90	U. Sinkewicz
 *		Performance enhancements to uniprocessor kernel.
 *
 *	30-May-89	U. Sinkewicz
 *		Added so->ref and SO_LOCK to fix the smp problem caused
 *		by unlocking the socket and then locking it again to
 *		accommodate the lock hierarchy and sleeps.  Changed
 *		call interface to ip_output() to support asymmetric
 *		network drivers.
 *
 *      05-May-89       Michael G. Mc Menemy
 *              Add XTI support.
 *
 *	27-Mar-89	U. Sinkewicz
 *		Lowered ipl on lk_rtentry, lk_ifnet, lk_in_ifaddr; replaced
 *		tcpstatictics with a macro as per lp changes made 3.16.89.
 *
 *	3-Mar-89	U. Sinkewicz
 *		Added new directory layout to smp file.
 *
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes.
 *
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	tcp_subr.c	6.6 (Berkeley) 6/8/85
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/smp_lock.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/errno.h"

#include "../net/net/route.h"
#include "../net/net/if.h"

#include "../net/netinet/in.h"
#include "../net/netinet/in_pcb.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"
#include "../net/netinet/ip_icmp.h"
#include "../net/netinet/tcp.h"
#include "../net/netinet/tcp_fsm.h"
#include "../net/netinet/tcp_seq.h"
#include "../net/netinet/tcp_timer.h"
#include "../net/netinet/tcp_var.h"
#include "../net/netinet/tcpip.h"
#ifdef XTI
#include "../h/file.h"
#endif XTI

int tcp_ttl = TCP_TTL;

/*
 * Tcp initialization.
 * SMP: Initialize the tcb smp lock and the tcpstat smp lock.
 */
struct lock_t lk_tcb;		/* SMP */
struct lock_t lk_tcpstat;	/* SMP */

tcp_init()
{
	int s;

	s = splnet(); /* SMP */
	smp_lock(&lk_tcpiss, LK_RETRY);
	tcp_iss = 1;		/* wrong */
	smp_unlock(&lk_tcpiss);
	splx(s);
	lockinit(&lk_tcb, &lock_tcb_d);
	lockinit(&lk_tcpstat, &lock_tcpstat_d);
	s = splnet(); /* SMP */
	smp_lock(&lk_tcb, LK_RETRY);
	tcb.inp_next = tcb.inp_prev = &tcb;
	smp_unlock(&lk_tcb);
	splx(s);
#ifdef XTI
	/* set-up XTI default characteristics */
	/* stored per provider */

	xti_tcpinfo.addr = sizeof(struct sockaddr_in);
	xti_tcpinfo.options = sizeof(struct tcp_options);
	xti_tcpinfo.tsdu = 0;
	xti_tcpinfo.etsdu = -1;
	xti_tcpinfo.connect = -2;
	xti_tcpinfo.discon = -2;
	xti_tcpinfo.servtype = T_COTS_ORD;
#endif XTI
}

/*
 * Create template to be used to send tcp packets on a connection.
 * Call after host entry created, allocates an mbuf and fills
 * in a skeletal tcp/ip header, minimizing the amount of work
 * necessary when the connection is used.
 */
/* SMP: lock held coming in. */
struct tcpiphdr *
tcp_template(tp)
	struct tcpcb *tp;
{
	register struct inpcb *inp = tp->t_inpcb;
	register struct mbuf *m;
	register struct tcpiphdr *n;
	if (smp_debug){
		if (smp_owner(&inp->inp_socket->lk_socket) == 0)
			panic("tcp_template not lock owner");
	}
	if((n = tp->t_template) == 0) {
		m = m_get(M_DONTWAIT, MT_DATA);
		if (m == NULL)
			return (0);
		m->m_off = MMAXOFF - sizeof (struct tcpiphdr);
		m->m_len = sizeof (struct tcpiphdr);
		n = mtod(m, struct tcpiphdr *);
	}
	n->ti_next = n->ti_prev = 0;
	n->ti_x1 = 0;
	n->ti_pr = IPPROTO_TCP;
	n->ti_len = htons(sizeof (struct tcpiphdr) - sizeof (struct ip));
	n->ti_src = inp->inp_laddr;
	n->ti_dst = inp->inp_faddr;
	n->ti_sport = inp->inp_lport;
	n->ti_dport = inp->inp_fport;
	n->ti_seq = 0;
	n->ti_ack = 0;
	n->ti_x2 = 0;
	n->ti_off = 5;
	n->ti_flags = 0;
	n->ti_win = 0;
	n->ti_sum = 0;
	n->ti_urp = 0;
	return (n);
}

/*
 * Send a single message to the TCP at address specified by
 * the given TCP/IP header.  If flags==0, then we make a copy
 * of the tcpiphdr at ti and send directly to the addressed host.
 * This is used to force keep alive messages out using the TCP
 * template for a connection tp->t_template.  If flags are given
 * then we send a message back to the TCP which originated the
 * segment ti, and discard the mbuf containing it and any other
 * attached mbufs.
 *
 * In any case the ack and sequence number of the transmitted
 * segment are as specified by the parameters.
 *
 * Enter from tcp_input and tcp_timer.  No locks set going in.
 */
/* Need to pass down so pointer to set the locks
 * properly.  Tp pointer is not sufficient to determine locking
 * because so could be locked and tp could be zero. 
 */
/* tcp_respond(tp, ti, ack, seq, flags) */
tcp_respond(tp, ti, ack, seq, flags, so) 
	struct tcpcb *tp;
	register struct tcpiphdr *ti;
	tcp_seq ack, seq;
	int flags;
	struct socket *so;
{
	register struct mbuf *m;
	int win = 0, tlen;
	struct route *ro = 0;

	/*
	 * SMP: tp may be NULL;  Check for smp
	 * lock ownership only if tp is okay
	 */
	if (tp) {
		win = sbspace(&tp->t_inpcb->inp_socket->so_rcv);
		ro = &tp->t_inpcb->inp_route;
	}
	if (flags == 0) {
		m = m_get(M_DONTWAIT, MT_DATA);
		if (m == NULL){
			return;
		}
#ifndef TCP_43BSD
		tlen = 1;
#else
		tlen = 0;
#endif
		m->m_len = sizeof (struct tcpiphdr) + 1;
		*mtod(m, struct tcpiphdr *) = *ti;
		ti = mtod(m, struct tcpiphdr *);
		flags = TH_ACK;
	} else {
		m = dtom(ti);
		m_freem(m->m_next);
		m->m_next = 0;
		m->m_off = (int)ti - (int)m;
		tlen = 0;
		m->m_len = sizeof (struct tcpiphdr);
#define xchg(a,b,type) { type t; t=a; a=b; b=t; }
		xchg(ti->ti_dst.s_addr, ti->ti_src.s_addr, u_long);
		xchg(ti->ti_dport, ti->ti_sport, u_short);
#undef xchg
	}
	ti->ti_next = ti->ti_prev = 0;
	ti->ti_x1 = 0;
	ti->ti_len = htons((u_short)(sizeof (struct tcphdr) + tlen));
	ti->ti_seq = htonl(seq);
	ti->ti_ack = htonl(ack);
	ti->ti_x2 = 0;
	ti->ti_off = sizeof (struct tcphdr) >> 2;
	ti->ti_flags = flags;
	ti->ti_win = htons((u_short)win);
	ti->ti_urp = 0;
#ifdef TCPLOOPBACK
	if (ti->ti_src.s_addr == ti->ti_dst.s_addr) {
		/* Fake IP transit and get to TCP layer fast */
		((struct ip *)ti)->ip_hl = sizeof (struct ip) >> 2;
		((struct ip *)ti)->ip_len = sizeof (struct tcphdr) + tlen;
		if (so){
			smp_unlock(&so->lk_socket);
		}
		tcp_input(m);
		if (so){
			SO_LOCK(so);
		}
	} else {
#endif TCPLOOPBACK
		ti->ti_sum = in_cksum(m, sizeof (struct tcpiphdr) + tlen);
		((struct ip *)ti)->ip_len = sizeof (struct tcpiphdr) + tlen;
		((struct ip *)ti)->ip_ttl = tcp_ttl;
		(void) ip_output(m, (struct mbuf *)0, ro, 0, so);
#ifdef TCPLOOPBACK
	}
#endif TCPLOOPBACK
}

/*
 * Create a new TCP control block, making an
 * empty reassembly queue and hooking it to the argument
 * protocol control block.
 */
/* SMP:  Note that inp is locked coming in; socket is locked 
 * coming in.
 */
struct tcpcb *
tcp_newtcpcb(inp)
	struct inpcb *inp;
{
#ifdef old
	struct mbuf *m = m_getclr(M_DONTWAIT, MT_PCB);
#endif
	register struct tcpcb *tp;

#ifdef old
	if (smp_debug){
		if (smp_owner(&inp->inp_socket->lk_socket) == 0)
			panic(" tcp_newtcpcb not lock owner");
	}
	if (m == NULL)
		return ((struct tcpcb *)0);
	tp = mtod(m, struct tcpcb *);
#endif
	KM_ALLOC(tp, struct tcpcb *, sizeof(struct tcpcb), KM_PCB, KM_CLEAR|KM_NOWAIT);
	if(tp == NULL)
		return((struct tcpcb *)0);
	tp->seg_next = tp->seg_prev = (struct tcpiphdr *)tp;
	tp->t_maxseg = TCP_MSS;
	tp->t_flags = 0;		/* sends options! */
	tp->t_inpcb = inp;
	/*
	 * Init srtt to TCPTV_SRTTBASE (0), so we can tell that we have no
	 * rtt estimate.  Set rttvar so that srtt + 2 * rttvar gives
	 * reasonable initial retransmit time.
	 */
	tp->t_srtt = TCPTV_SRTTBASE;
	tp->t_rttvar = TCPTV_SRTTDFLT << 2;
	TCPT_RANGESET(tp->t_rxtcur, 
	    ((TCPTV_SRTTBASE >> 2) + (TCPTV_SRTTDFLT << 2)) >> 1,
	    TCPTV_MIN, TCPTV_REXMTMAX);
	tp->snd_cwnd = sbspace(&inp->inp_socket->so_snd);
	tp->snd_ssthresh = 65535;		/* XXX */
	inp->inp_ppcb = (caddr_t)tp;
#ifdef XTI
	/* set-up default tcp options */

	tp->xti_tcp_defqos.precedence = T_ROUTINE;
	tp->xti_tcp_defqos.timeout = (TCP_LINGERTIME*1000);
	tp->xti_tcp_defqos.max_seg_size = tp->t_maxseg;
	tp->xti_tcp_defqos.secopt.security = T_UNUSED;
	tp->xti_tcp_defqos.secopt.compartment = T_UNUSED;
	tp->xti_tcp_defqos.secopt.handling = T_UNUSED;
	tp->xti_tcp_defqos.secopt.tcc = T_UNUSED;
	
	/* load default options into negotiated area */

	tp->xti_neg_qos = tp->xti_tcp_defqos;

#endif XTI
	return (tp);
}

/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */
struct tcpcb *
tcp_drop(tp, errno)
	register struct tcpcb *tp;
	int errno;
{
	struct socket *so = tp->t_inpcb->inp_socket;
	int s;

	if (smp_debug){
		if (smp_owner(&so->lk_socket) == 0)
			panic("tcp_drop not lock owner");
	}
	if (TCPS_HAVERCVDSYN(tp->t_state)) {
		tp->t_state = TCPS_CLOSED;
		(void) tcp_output(tp);
		TCPSTAT(tcps_drops++);
	} else{
		TCPSTAT(tcps_conndrops++);
	}
	so->so_error = errno;
	return (tcp_close(tp));
}

/*
 * Close a TCP control block:
 *	discard all space held by the tcp
 *	discard internet protocol block
 *	wake up any sleepers
 */
/* SMP:  Enter with a socket lock set.
 */
struct tcpcb *
tcp_close(tp)
	register struct tcpcb *tp;
{
	register struct tcpiphdr *t;
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
	register struct mbuf *m;
	int s; 				/* SMP */
	struct socket *so_tmp = NULL;	/* SMP */
	int status = 0;			/* SMP */
	struct inpcb *inp_tmp = NULL;	/* SMP */
	struct socket *so_addr = NULL;	/* SMP */

	if (smp_debug){
		if (smp_owner(&so->lk_socket) == 0)
			panic("tcp_close not lock owner");
	}

	t = tp->seg_next;
	while (t != (struct tcpiphdr *)tp) {
		t = (struct tcpiphdr *)t->ti_next;
		m = dtom(t->ti_prev);
		remque(t->ti_prev);
		m_freem(m);
	}
	if (tp->t_template)
		(void) m_free(dtom(tp->t_template));

	/* Note that the manipulation with owner and the lk_tcb here
	 * is due to the fact that when we are called from the timers,
	 * lk_tcb is already set. */
   if (smp){	
	if (smp_owner(&lk_tcb) == 1){
#ifdef old
		(void) m_free(dtom(tp));
#endif
		KM_FREE(tp, KM_PCB);
		inp->inp_ppcb = 0;
		soisdisconnected(so);
		in_pcbdetach(inp);
		TCPSTAT(tcps_closed++);
		return ((struct tcpcb *)0);
	}
	{
		int owner = 0;
	if (( so->so_head) && (smp_owner(&so->so_head->lk_socket)) == 1){
		owner = 1;
		so->so_head->ref = 28;
		smp_unlock(&so->so_head->lk_socket);
		}
	so->ref = 10;
	smp_unlock(&so->lk_socket);
	smp_lock(&lk_tcb, LK_RETRY);
	smp_lock(&so->lk_socket, LK_RETRY);
	so->ref = 0;
	if (owner){
		smp_lock(&so->so_head->lk_socket, LK_RETRY);
		so->so_head->ref = 0;
		owner = 0;
		}
	}
   }	
#ifdef old
	(void) m_free(dtom(tp));
#endif
	KM_FREE(tp, KM_PCB);
	inp->inp_ppcb = 0;
	soisdisconnected(so);
	in_pcbdetach(inp);
	smp_unlock(&lk_tcb);
	TCPSTAT(tcps_closed++);
	return ((struct tcpcb *)0);
}

tcp_drain()
{

}

/*
 * Notify a tcp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
tcp_notify(inp)
	register struct inpcb *inp;
{
	/* Ensure that socket is locked before
	 * doing wakeups because low level wakeup routines write
	 * to the socket.  
	 */
	/* Note that here, lk_tcb is already held so if we use
	 * SO_LOCK, then SO_LOCK could find so->ref busy and spin
	 * with lk_tcb held.  This could result in a starvation
	 * panic on lk_tcb.
	 */
	smp_lock(&inp->inp_socket->lk_socket, LK_RETRY);
	wakeup((caddr_t) &inp->inp_socket->so_timeo);
        sorwakeup(inp->inp_socket);
	sowwakeup(inp->inp_socket);
	smp_unlock(&inp->inp_socket->lk_socket);
}

tcp_ctlinput(cmd, sa)
	int cmd;
	struct sockaddr *sa;
{
	extern u_char inetctlerrmap[];
	struct sockaddr_in *sin;
	int tcp_quench(), in_rtchange();
	int s;

	if ((unsigned) cmd > PRC_NCMDS)
		return;
	if (sa->sa_family != AF_INET && sa->sa_family != AF_IMPLINK)
		return;
	sin = (struct sockaddr_in *)sa;
	if (sin->sin_addr.s_addr == INADDR_ANY)
		return;

	switch (cmd) {

	case PRC_QUENCH:
		s = splnet(); 
		smp_lock(&lk_tcb, LK_RETRY);
		in_pcbnotify(&tcb, &sin->sin_addr, 0, tcp_quench);
		smp_unlock(&lk_tcb);
		splx(s);
		break;

	case PRC_ROUTEDEAD:
	case PRC_REDIRECT_NET:
	case PRC_REDIRECT_HOST:
	case PRC_REDIRECT_TOSNET:
	case PRC_REDIRECT_TOSHOST:
		s = splnet(); /* SMP */
		smp_lock(&lk_tcb, LK_RETRY);
		in_pcbnotify(&tcb, &sin->sin_addr, 0, in_rtchange);
		smp_unlock(&lk_tcb);
		splx(s);
		break;

	default:
		if (inetctlerrmap[cmd] == 0)
			return;		/* XXX */
		s = splnet(); /* SMP */
		smp_lock(&lk_tcb, LK_RETRY);
		in_pcbnotify(&tcb, &sin->sin_addr, (int)inetctlerrmap[cmd], 
			tcp_notify);
		smp_unlock(&lk_tcb);
		splx(s);

	}
}

/*
 * When a source quench is received, close congestion window
 * to one segment.  We will gradually open it again as we proceed.
 */
tcp_quench(inp)
	struct inpcb *inp;
{
	struct tcpcb *tp = intotcpcb(inp);

	if(tp)
		tp->snd_cwnd = tp->t_maxseg;
}


#ifdef XTI
/*
 * KEY
 * ---
 * so   - socket pointer
 * inp  - internet control block pointer
 * tp   - tcp control block pointer
 * fd   - file desciptor
 *
 * STATE        MINIMUM MAPPING
 * ----------   -----------------------------
 * T_UNINIT     (getsock(fd) == 0))
 * T_UNBND      (getsock(fd) > 0)
 * T_IDLE	(inp->inp_lport || inp->inp_fport)
 * T_OUTCON     (so->so_state & SS_ISCONNECTING) 
 * T_INCON      (((so->so_options & SO_ACCEPTCONN) && so->so_qlen > 0) ||
 *              ((so->so_options & SO_ACCEPTCONN) && (so->so_state & SS_NBIO)))
 *              
 * T_DATAXFER   (so->so_state & SS_ISCONNECTED) 
 * T_OUTREL     ((so->so_state & SS_CANTSENDMORE) && 
 *	          (tp->t_state >= TCPS_FIN_WAIT_1))
 * T_INREL      ((!(so->so_state & SS_CANTSENDMORE)) && 
 *	          (tp->t_state >= TCPS_CLOSE_WAIT))
 */

tcp_to_xtistate(so)
     struct socket *so;
{

  int status;
  int default_state;
  struct inpcb *inp;
  struct tcpcb *tp;

  /*
   * determine what state we are in
   */

  /*
   * we are already past T_UNINIT state, because
   * if we were a bogus descriptor or a non-socket descriptor we
   * would have been caught.
   */

  default_state = T_UNBND;


  if (so) 
    inp = (struct inpcb *) so->so_pcb;
  if (inp)
    tp = (struct tcpcb *) inp->inp_ppcb;


  if (inp)
    if (inp->inp_lport || inp->inp_fport) {
      default_state = T_IDLE;
    } else {
      return(default_state);
    }
  else
    return(default_state);

  if (so) {
    if (so->so_state & SS_ISCONNECTING) {
      default_state = T_OUTCON;
    } else if ((so->so_options & SO_ACCEPTCONN) && so->so_qlen != 0) {
      default_state = T_INCON;
    } else if ((so->so_options & SO_ACCEPTCONN) && (so->so_state & SS_NBIO)) {
      default_state = T_INCON;
    } else if (so->so_state & SS_ISCONNECTED) {
      default_state = T_DATAXFER;
    }
  }
  
  if (tp) {
    if ((so->so_state & SS_CANTSENDMORE) && 
	(tp->t_state >= TCPS_FIN_WAIT_1)) {
      default_state = T_OUTREL;
    } else if ((!(so->so_state & SS_CANTSENDMORE)) &&
	       (tp->t_state >= TCPS_CLOSE_WAIT)) {
      default_state = T_INREL;
    }
  }
  return(default_state);
}

/*
 * Close a TCP control block:
 *	discard all space held by the tcp
 *	DONT discard internet protocol block
 *	wake up any sleepers
 */
/* SMP:  Remove tp pointer only if no other routines are using it.
 * Enter with socket lock set.
 */
struct tcpcb *
tcp_closekeepinp(tp)
	register struct tcpcb *tp;
{
	register struct tcpiphdr *t;
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
	register struct mbuf *m;
	int s;

	if (smp_debug){
		if (smp_owner(&so->lk_socket) == 0)
			panic("tcp_closekeepinp not lock owner");
	}

	t = tp->seg_next;
	while (t != (struct tcpiphdr *)tp) {
		t = (struct tcpiphdr *)t->ti_next;
		m = dtom(t->ti_prev);
		remque(t->ti_prev);
		m_freem(m);
	}
	if (tp->t_template)
		(void) m_free(dtom(tp->t_template));
	KM_FREE(tp, KM_PCB);
	inp->inp_ppcb = 0;
	TCPSTAT(tcps_closed++);
	return ((struct tcpcb *)0);
}

/*
 *	
 * cannot use bcmp because of padding
 * values may differ.
 */

int
  cmp_xti_tcpopts(src, dst)
struct tcp_options *src;
struct tcp_options *dst;
{
  int trace = 0;

#define CMPTCPOPT(member) \
  if (src->member != dst->member) { \
        return(trace); } \
  trace++;

  CMPTCPOPT(precedence); 
  CMPTCPOPT(timeout); 
  CMPTCPOPT(max_seg_size); 
  CMPTCPOPT(secopt.security); 
  CMPTCPOPT(secopt.compartment); 
  CMPTCPOPT(secopt.handling); 
  CMPTCPOPT(secopt.tcc); 
  return(0);
}
#endif XTI



