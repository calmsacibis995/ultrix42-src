#ifndef lint
static char *sccsid = "@(#)tcp_usrreq.c	4.3	ULTRIX	3/7/91";
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
 *      07-Mar-91       Michael G. Mc Menemy
 *              Fix OOB handling if SO_BUFFOOB is turned off
 *
 *	18-June-90	lp
 *		Bump up send/receive buffers for TCP. May use more
 * 		memory on ethernet but should speed transfers.
 *
 *	2-Jan-90	U. Sinkewicz
 *		Performance enhancements to uniprocessor kernel.
 *
 *	30-May-89	U. Sinkewicz
 *		Added so->ref and SO_LOCK to fix the smp problem caused
 *		by unlocking the socket and locking it again to 
 *		accommodate the lock hierarchy and sleeps.
 *
 *      05-May-89       Michael G. Mc Menemy
 *              Add XTI support.
 *
 *	31-Mar-89	U. Sinkewicz
 *		Change lock heirarchy so that lk_tcb is lower than
 *		lk_socket.
 *
 *	27-Mar-89	U. Sinkewicz
 *		Replaced tcpstatistics with a macro as per lp changes
 *		from 3.16.89.
 *
 *	3-Mar-89	U. Sinkewicz
 *		Pmax/smp merge.
 *	
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes.
 *
 *	Larry Cohen  -  01/28/87
 *		Add tcp control output routine
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
 *	tcp_usrreq.c	6.6 (Berkeley) 6/8/85
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/errno.h"
#include "../h/stat.h"
#include "../h/smp_lock.h"

#include "../net/net/if.h"
#include "../net/net/route.h"
#ifdef XTI
#include "../h/domain.h"
#endif XTI
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

/*
 * TCP protocol interface to socket abstraction.
 */
extern	char *tcpstates[];
struct	tcpcb *tcp_newtcpcb();
int	tcpsenderrors;
#ifdef XTI
extern int xti_debug;
#define PRINTXTID(level, msg)   \
  /*                            \
   * level:                     \
   * 0x01 = generate events;    \
   * 0x04 = acceptchk/abort support;  \
   * 0x08 = peek events;        \
   * 0x10 = tpdu T_MORE support; \
   * 0x20 = oob mark;           \
   * 0x40 = options negot.      \
   * 0x80 = protocol;           \
   */                           \
  if (xti_debug & (level))      \
     cprintf((msg))
#else
#define PRINTXTID(level, msg)
#endif XTI

/*
 * Process a TCP user request for TCP tb.  If this is a send request
 * then m is the mbuf chain of send data.  If this is a timer expiration
 * (called from the software clock routine), then timertype tells which timer.
 * SMP: Enter with socket lock set except for PRU_SENSE which is called from
 * file system.
 */
/*ARGSUSED*/
tcp_usrreq(so, req, m, nam, rights)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	register struct inpcb *inp;
	register struct tcpcb *tp;
	int s;
	int error = 0;
	int ostate;
	struct socket *so_tmp = NULL;		/* SMP */
	struct socket *so_addr = NULL;		/* SMP */
	int status = 0;				/* SMP */
	struct inpcb *inp_tmp = NULL;		/* SMP */

	/* 
 	 * For CONTROL, come in at splnet with socket locked.  Path is from
	 * ioctl (sys_generic.c) to soo_ioctl (sys_socket.c) to ifioctl (if.c)
	 * to tcp_usrreq (pr->pr_usrreq).  Locks are set in 
	 * soo_ioctl because it changes socket state fields.  3.26.87.us
	 *
	 */
	if (req == PRU_CONTROL)
		return (in_control(so, (int)m, (caddr_t)nam,
			(struct ifnet *)rights));
	if (rights && rights->m_len)
		return (EINVAL);
	s = splnet();
	inp = sotoinpcb(so);
	/*
	 * When a TCP is attached to a socket, then there will be
	 * a (struct inpcb) pointed at by the socket, and this
	 * structure will point at a subsidary (struct tcpcb).
	 */
	if (inp == 0 && req != PRU_ATTACH) {
		return (EINVAL);		/* XXX */
	}
	if (inp) {
		tp = intotcpcb(inp);
		/* WHAT IF TP IS 0? */
#ifdef XTI
		if (tp == 0) {
			return(EBADF);
		}
#endif XTI
#ifdef KPROF
		tcp_acounts[tp->t_state][req]++;
#endif
		ostate = tp->t_state;
	} else
		ostate = 0;
	switch (req) {

	/*
	 * TCP attaches to socket via PRU_ATTACH, reserving space,
	 * and an internet control block.
	 */
	case PRU_ATTACH:
		if (inp) {
			error = EISCONN;
			break;
		}
		error = tcp_attach(so);
		if (error)
			break;
		if ((so->so_options & SO_LINGER) && so->so_linger == 0)
			so->so_linger = TCP_LINGERTIME;
		tp = sototcpcb(so);
#ifdef XTI
		so->so_xticb.xti_tpinfo = &xti_tcpinfo;
#endif XTI
		break;

	/*
	 * PRU_DETACH detaches the TCP protocol from the socket.
	 * If the protocol state is non-embryonic, then can't
	 * do this directly: have to initiate a PRU_DISCONNECT,
	 * which may finish later; embryonic TCB's can just
	 * be discarded here.
	 */
	case PRU_DETACH:
		if (tp->t_state > TCPS_LISTEN)
			tp = tcp_disconnect(tp);
		else
			tp = tcp_close(tp);
		break;

	/*
	 * Give the socket an address.
	 */
	case PRU_BIND:
		if (smp){
			so->ref = 11;
			smp_unlock(&so->lk_socket);
			smp_lock(&lk_tcb, LK_RETRY);
			smp_lock(&so->lk_socket, LK_RETRY);
			so->ref = 0;
			error = in_pcbbind(inp, nam);
			smp_unlock(&lk_tcb);
		}
		else
			error = in_pcbbind(inp, nam);
		if (error)
			break;
		break;

	/*
	 * Prepare to accept connections.
	 */
	case PRU_LISTEN:
		if (inp->inp_lport == 0){
			if(smp){
				so->ref = 12;
				smp_unlock(&so->lk_socket);
				smp_lock(&lk_tcb, LK_RETRY);
				smp_lock(&so->lk_socket, LK_RETRY);
				so->ref = 0;
				error = in_pcbbind(inp, (struct mbuf *)0);
				smp_unlock(&lk_tcb);
			}else
				error = in_pcbbind(inp, (struct mbuf *)0);
		}
		if (error == 0)
			tp->t_state = TCPS_LISTEN;
		break;

	/*
	 * Initiate connection to peer.
	 * Create a template for use in transmissions on this connection.
	 * Enter SYN_SENT state, and mark socket as connecting.
	 * Start keep-alive timer, and seed output sequence space.
	 * Send initial segment on connection.
	 */
	/* SMP: socket locked coming in. */
	case PRU_CONNECT:
		if (inp->inp_lport == 0) {
			if (smp){
				so->ref = 13;
				smp_unlock(&so->lk_socket);
				smp_lock(&lk_tcb, LK_RETRY);
				smp_lock(&so->lk_socket, LK_RETRY);
				so->ref = 0;
				error = in_pcbbind(inp, (struct mbuf *)0);
				smp_unlock(&lk_tcb);
			}else
				error = in_pcbbind(inp, (struct mbuf *)0);
			if (error)
				break;
		}
		if (smp){
			so->ref = 13;
			smp_unlock(&so->lk_socket);
			smp_lock(&lk_tcb, LK_RETRY);
			smp_lock(&so->lk_socket, LK_RETRY);
			so->ref = 0;
			error = in_pcbconnect(inp, nam);
			smp_unlock(&lk_tcb);
		}
		else
			error = in_pcbconnect(inp, nam);
		if (error)
			break;
		tp->t_template = tcp_template(tp);
		if (tp->t_template == 0) {
			if (smp){
				so->ref = 13;
				smp_unlock(&so->lk_socket);
				smp_lock(&lk_tcb, LK_RETRY);
				smp_lock(&so->lk_socket, LK_RETRY);
				so->ref = 0;
				in_pcbdisconnect(inp);
				smp_unlock(&lk_tcb);
			}else
				in_pcbdisconnect(inp);
			error = ENOBUFS;
			break;
		}
		soisconnecting(so);	/* Does a wakeup */
		TCPSTAT(tcps_connattempt++);
		tp->t_state = TCPS_SYN_SENT;
		tp->t_timer[TCPT_KEEP] = TCPTV_KEEP;
		smp_lock(&lk_tcpiss, LK_RETRY);
		tp->iss = tcp_iss; tcp_iss += TCP_ISSINCR/2;
		smp_unlock(&lk_tcpiss);
		tcp_sendseqinit(tp);
		error = tcp_output(tp);
		break;

	/*
	 * Create a TCP connection between two sockets.
	 */
	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	/*
	 * Initiate disconnect from peer.
	 * If connection never passed embryonic stage, just drop;
	 * else if don't need to let data drain, then can just drop anyways,
	 * else have to begin TCP shutdown process: mark socket disconnecting,
	 * drain unread data, state switch to reflect user close, and
	 * send segment (e.g. FIN) to peer.  Socket will be really disconnected
	 * when peer sends FIN and acks ours.
	 *
	 * SHOULD IMPLEMENT LATER PRU_CONNECT VIA REALLOC TCPCB.
	 */
	case PRU_DISCONNECT:
		tp = tcp_disconnect(tp);
		break;

	/*
	 * Accept a connection.  Essentially all the work is
	 * done at higher levels; just return the address
	 * of the peer, storing through addr.
	 */
	case PRU_ACCEPT: {
		struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);

		nam->m_len = sizeof (struct sockaddr_in);
		sin->sin_family = AF_INET;
		sin->sin_port = inp->inp_fport;
		sin->sin_addr = inp->inp_faddr;
		break;
		}

	/*
	 * Mark the connection as being incapable of further output.
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		tp = tcp_usrclosed(tp);
		if (tp)
			error = tcp_output(tp);
		break;

	/*
	 * After a receive, possibly send window update to peer.
	 */
	case PRU_RCVD:
		(void) tcp_output(tp);
		break;

	/*
	 * Do a send by putting data in output queue and updating urgent
	 * marker if URG set.  Possibly send more data.
	 * SMP: socket lock set coming in.
	 */
	case PRU_SEND:
		sbappend(&so->so_snd, m);
		error = tcp_output(tp);
		if (error) {		/* XXX fix to use other path */
			if (error == ENOBUFS)		/* XXX */
				error = 0;		/* XXX */
			tcpsenderrors++;
		}
		break;

	/*
	 * Abort the TCP.
	 */
	case PRU_ABORT:
		tp = tcp_drop(tp, ECONNABORTED);
		break;

	case PRU_SENSE:
		((struct stat *) m)->st_blksize = so->so_snd.sb_hiwat;
		(void)splx(s);
		return (0);

	case PRU_RCVOOB:
		if ((so->so_oobmark == 0 &&
		    (so->so_state & SS_RCVATMARK) == 0) ||
		    so->so_options & SO_OOBINLINE ||
		    tp->t_oobflags & TCPOOB_HADDATA) {
			error = EINVAL;
			break;
		}
		if ((tp->t_oobflags & TCPOOB_HAVEDATA) == 0) {
			error = EWOULDBLOCK;
			break;
		}

		if (so->so_options & SO_BUFFOOB) {
		  if (((int)nam & MSG_PEEK) == 0)
		    if (!so->so_exrcv.sb_cc)
		      tp->t_oobflags ^= (TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		} else {
		  m->m_len = 1;
		  *mtod(m, caddr_t) = tp->t_iobc;
		  if (((int)nam & MSG_PEEK) == 0)
		    tp->t_oobflags ^= (TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		}
		break;

	case PRU_SENDOOB:
		if (sbspace(&so->so_snd) < -512) {
			m_freem(m);
			error = ENOBUFS;
			break;
		}
		/*
		 * According to RFC961 (Assigned Protocols),
		 * the urgent pointer points to the last octet
		 * of urgent data.  We continue, however,
		 * to consider it to indicate the first octet
		 * of data past the urgent section.
		 * Otherwise, snd_up should be one lower.
		 */
		sbappend(&so->so_snd, m);
		tp->snd_up = tp->snd_una + so->so_snd.sb_cc;
		tp->t_force = 1;
		error = tcp_output(tp);
		tp->t_force = 0;
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, nam);
		break;

	/*
	 * TCP slow timer went off; going through this
	 * routine for tracing's sake.
	 */
	/* SMP: Lock set in tcp_slowtimeo. 4.21.87.us. */
	case PRU_SLOWTIMO:
		tp = tcp_timers(tp, (int)nam);
		req |= (int)nam << 8;		/* for debug's sake */
		break;

	default:
		error = EOPNOTSUPP;
	}
	if (tp && (so->so_options & SO_DEBUG))
		tcp_trace(TA_USER, ostate, tp, (struct tcpiphdr *)0, req);
	splx(s);
	return (error);
}

tcp_ctloutput(op, so, level, optname, mp)
	int op;
	struct socket *so;
	int level, optname;
	struct mbuf **mp;
{
#ifdef XTI

	int error = 0;
	struct inpcb *inp = 0;
	register struct tcpcb *tp = 0;
	register struct mbuf *m;
	int s;
  
	inp = sotoinpcb(so);
	if (inp == NULL) {
	  if (so->so_error)
	    error = so->so_error;
	  else
	    error = ENOPROTOOPT;

	  m = *mp; /* get ready to free it */
	  goto badcb;
	}

	tp = intotcpcb(inp);

	if (tp == NULL) {
	  if (so->so_error)
	    error = so->so_error;
	  else
	    error = ENOPROTOOPT;
	  m = *mp; /* get ready to free it */
	  goto badcb;
	}

#else
	int error = 0;
	struct inpcb *inp = sotoinpcb(so);
	register struct tcpcb *tp = intotcpcb(inp);
	register struct mbuf *m;
#endif XTI
	if (level != IPPROTO_TCP)
		return (ip_ctloutput(op, so, level, optname, mp));

	switch (op) {

	case PRCO_SETOPT:
		m = *mp;
		switch (optname) {

		case TCP_NODELAY:
			if (m == NULL || m->m_len < sizeof (int))
				error = EINVAL;
			else if (*mtod(m, int *))
				tp->t_flags |= TF_NODELAY;
			else
				tp->t_flags &= ~TF_NODELAY;
			break;

		case TCP_MAXSEG:	/* not yet */
			error = EINVAL;
			break;
#ifdef XTI
		case TCP_CONOPT:
			{
			  int tmp_var;
			  char *src_ptr;
			  char *dst_ptr;
			  unsigned short calculate_mss;

			  if (m == NULL || m->m_len < sizeof(struct tcp_options)) {
			    error = EINVAL;
			    break;
			  }
	
			  calculate_mss = MIN(so->so_rcv.sb_hiwat / 2, tcp_mss(tp));
			  PRINTXTID(64, ("calmss=%d\n",calculate_mss));
			  src_ptr = (char *) (mtod(m, struct tcp_options *));
			  dst_ptr = (char *) &tp->xti_neg_qos;
	
			  /*
			   *	Only allow maximum segment size to be changed
			   */

			  if (((struct tcp_options *)src_ptr)->max_seg_size 
			      <= calculate_mss) {

			    tp->xti_neg_qos.max_seg_size =
			      ((struct tcp_options *)src_ptr)->max_seg_size;

			  }
#ifdef undef
			  tmp_var = bcmp((char *)(mtod(m, struct tcp_options *)),
					 (char *) &tp->xti_neg_qos,
					 sizeof(struct tcp_options));
#endif			  
			  tmp_var = cmp_xti_tcpopts(
				       (char *)(mtod(m, struct tcp_options *)),
				       (char *) &tp->xti_neg_qos);


			  PRINTXTID(64, ("qos:prec,timeout,max_seg,sec,com,hand,ttc=%x %x %d %x %x % %x\n",
					 tp->xti_neg_qos.precedence,
					 tp->xti_neg_qos.timeout,
					 tp->xti_neg_qos.max_seg_size,
					 tp->xti_neg_qos.secopt.security,
					 tp->xti_neg_qos.secopt.compartment,
					 tp->xti_neg_qos.secopt.handling,
					 tp->xti_neg_qos.secopt.tcc));
			  PRINTXTID(64, ("tqos:prec,timeout,max_seg,sec,com,hand,ttc=%x %x %d %x %x %x %x\n",
					 ((struct tcp_options *)src_ptr)->precedence,
					 ((struct tcp_options *)src_ptr)->timeout,
					 ((struct tcp_options *)src_ptr)->max_seg_size,
					 ((struct tcp_options *)src_ptr)->secopt.security,
					 ((struct tcp_options *)src_ptr)->secopt.compartment,
					 ((struct tcp_options *)src_ptr)->secopt.handling,
					 ((struct tcp_options *)src_ptr)->secopt.tcc));
			  if (tmp_var) {
			    error = EOPNOTSUPP;
			    break;
			  }
			  break;
			}
      
	        case TCP_NEGQOS:
      
		if (m == NULL || m->m_len < sizeof(struct tcp_options)) {
		  error = EOPNOTSUPP;
		  break;
		}
		  
		tp->xti_tmpneg_qos = (*mtod(m, struct tcp_options *));
		  
		break;

	        case TCP_CHKQOS:
		{
		  int tmp_var;
		  char *src_ptr;
		  char *dst_ptr;
		    
		  if (m == NULL || m->m_len < sizeof(struct tcp_options)) {
		    error = EINVAL;
		    break;
		  }
		    
		  src_ptr = (char *) (mtod(m, struct tcp_options *));
		  dst_ptr = (char *) &tp->xti_neg_qos;
#ifdef undef	
		  tmp_var = bcmp((char *)(mtod(m, struct tcp_options *)),
				 (char *) &tp->xti_neg_qos,
				 sizeof(struct tcp_options));
#endif
		  tmp_var = cmp_xti_tcpopts(
				   (char *)(mtod(m, struct tcp_options *)),
				   (char *) &tp->xti_neg_qos);
		    

		  PRINTXTID(64, ("tmp_var = %d\n",tmp_var));
		  PRINTXTID(64, ("src = %x, dst = %x, size = %d\n",
				 (char *)(mtod(m,struct tcp_options *)),
				 (char *) &tp->xti_neg_qos,
				 sizeof(struct tcp_options)));

		  if (tmp_var) {
		    error = EOPNOTSUPP;
		    break;
		  }
		  break;
		}
		  

                case TCP_CONACCEPT:  
		  
		tp->t_flags |= TF_ACKNOW;
		(void) tcp_output(tp);
		break;
		  
                case TCP_ACCEPTMODE:  /* enable/disable accept mode */
		  
		if (m == NULL || m->m_len < sizeof (int))
		  error = EINVAL;
		else
		  tp->t_acceptmode = (*mtod(m, int *));
		break;
		  
#endif XTI

		default:
			error = EINVAL;
			break;
		}
#ifdef XTI
badcb:
#endif XTI
		if(m)
			(void) m_free(m);
		break;

	case PRCO_GETOPT:
		*mp = m = m_get(M_DONTWAIT, MT_SOOPTS);  /* SMP */
		if (m == NULL)
			return(ENOBUFS);
		m->m_len = sizeof(int);

		switch (optname) {
		case TCP_NODELAY:
			*mtod(m, int *) = tp->t_flags & TF_NODELAY;
			break;
		case TCP_MAXSEG:
			*mtod(m, int *) = tp->t_maxseg;
			break;
#ifdef XTI
		case TCP_CONOPT:
      
			m->m_len = sizeof(struct tcp_options);
			(*mtod(m, struct tcp_options *)) = tp->xti_inbound_qos;
			break;
      
		case TCP_NEGQOS:
      
			/* NEGOTIATION WOULD GO HERE....
			 * Currently only support 
			 * default values
			 */
      
			m->m_len = sizeof(struct tcp_options);
			(*mtod(m, struct tcp_options *)) = tp->xti_neg_qos;
			break;
      
      
		case TCP_DFLTQOS:
      
			m->m_len = sizeof(struct tcp_options);
			(*mtod(m, struct tcp_options *)) = tp->xti_tcp_defqos;
			break;

#endif XTI
		default:
			error = EINVAL;
			break;
		}
		break;
#ifdef XTI

        case PRCO_XTIMAPSTATE:
		so->so_xticb.xti_states = tcp_to_xtistate(so);
		break;

	case PRCO_XTIMAPINFO:
	
		so->so_xticb.xti_tpinfo = 
		  &xti_tcpinfo; /* info is per provider */
		break;

        case PRCO_XTICHKADDR:
		{
		  struct xti_accept_check {
		    int resfd; /* resfd of t_accept */
		    int seqnum; /* sequence number of t_accept */
		    union {
		      struct sockaddr generic; /* address info from t_accept */
		    } addr;
		  } tmp_accept;
		  
		  struct sockaddr_in tmp_addr;
		  struct inpcb *inp;
		  struct sockaddr_in *adeb;
		  
		  m = *mp; /* get mbuf pointer */
		  tmp_accept = *mtod(m, struct xti_accept_check *);
		  inp = sotoinpcb(so);
		  tmp_addr.sin_family = so->so_proto->pr_domain->dom_family;
		  tmp_addr.sin_port = inp->inp_fport;
		  tmp_addr.sin_addr = inp->inp_faddr;
		  bzero((char *) &tmp_addr.sin_zero[0], 8); /* char sin_zero[8] */
		  adeb = (struct sockaddr_in *) &tmp_accept.addr.generic;
		  bzero((char *) &adeb->sin_zero[0], 8); /* char sin_zero[8] */
		  
		  if (bcmp((char *)&tmp_accept.addr.generic,
			   (char *)&tmp_addr,
			   sizeof(struct sockaddr_in))) {
		    return(EADDRNOTAVAIL);
		  }
		  break;
		}
	  
        case PRCO_XTIREJECT:
		{
		  
		  tp = tcp_drop(tp, ECONNABORTED);
		  break;
		}
		break;
	  
        case PRCO_XTICOPYTP:
		{

		  struct socket *new = 0;
		  m = *mp;
		  new = *mtod(m, struct socket **);

		  sototcpcb(new)->t_acceptmode
		    = sototcpcb(so)->t_acceptmode;
		  sototcpcb(new)->xti_neg_qos
		    = sototcpcb(so)->xti_neg_qos;
		  sototcpcb(new)->xti_tmpneg_qos
		    = sototcpcb(so)->xti_tmpneg_qos;
		  sototcpcb(new)->xti_tcp_defqos
		    = sototcpcb(so)->xti_tcp_defqos;
		  break;
		}

        case PRCO_XTIUNBIND:
		{

		  xtiin_pcbunbind(so->so_pcb);
		  if (((struct inpcb *)(so->so_pcb))->inp_ppcb)
		    tcp_closekeepinp(((struct inpcb *) (so->so_pcb))->inp_ppcb);

		  tp = tcp_newtcpcb(inp);
		  if (tp == 0) {
		    int nofd = so->so_state & SS_NOFDREF;	/* XXX */
	
		    so->so_state &= ~SS_NOFDREF;	/* don't free the socket yet */
		    smp_lock(&lk_tcb, LK_RETRY);
		    in_pcbdetach(inp);
		    smp_unlock(&lk_tcb);
		    so->so_state |= nofd;
		    error = ENOBUFS;
		  }
		  break;
		}

#endif XTI


	}
	return (error);
}

/*
 * Allow buffering of 16K of data. This trades off memory for fewer 
 * acks by tcp. - lp
 */
int	tcp_sendspace = 1024*16;
int	tcp_recvspace = 1024*16;
#ifdef XTI
int     tcp_exrecvspace = 16*1;
#endif XTI
/*
 * Attach TCP protocol to socket, allocating
 * internet protocol control block, tcp control block,
 * bufer space, and entering LISTEN state if to accept connections.
 *
 * SMP: Lock set coming in.  4.14.87.us
 */
tcp_attach(so)
	struct socket *so;
{
	register struct tcpcb *tp;
	struct inpcb *inp;
	int error;
	int owner = 0; /* SMP */

	error = soreserve(so, tcp_sendspace, tcp_recvspace);
	if (error)
		return (error);
#ifdef XTI
	so->so_options |= SO_BUFFOOB;
	error = soexreserve(so, 0, tcp_exrecvspace);
	if (error)
	  return (error);
#endif XTI

	/* SMP: Lock queue going into in_pcballoc because can be
	 * used for different queues.  Bug fix 6.3.87.us */
        /* SMP: Bug fix.  3.22.89.us.  
         * The situation here is that tcp_attach can be called with one
         * socket locked (as in from socket()) OR with two sockets
         * locked (as in from sonewconn).  Since the bug fix here
         * is to exchange the heirarchial positions of the lk_socket
         * and lk_tcb, we must unlock socket(s) before locking
         * lk_tcb.  Therefore, we have the check to see if we
         * have only one or two sockets locked.
         */
        if((smp) && (so->so_head) && (smp_owner(&so->so_head->lk_socket) == 1)){
                owner = 1;
		so->so_head->ref = 29;
                smp_unlock(&so->so_head->lk_socket);
        }
	if (smp){
		so->ref = 14;
	        smp_unlock(&so->lk_socket);
		smp_lock(&lk_tcb, LK_RETRY);
		smp_lock(&so->lk_socket, LK_RETRY);
		so->ref = 0;
	        if (owner){
	                        smp_lock(&so->so_head->lk_socket, LK_RETRY);
				owner = 0;
				so->so_head->ref = 0;
	                }
		error = in_pcballoc(so, &tcb);
		smp_unlock(&lk_tcb);
	}else
		error = in_pcballoc(so, &tcb);
	if (error)
		return (error);
	inp = sotoinpcb(so);
	tp = tcp_newtcpcb(inp);
	if (tp == 0) {
		int nofd = so->so_state & SS_NOFDREF;	/* XXX */

		so->so_state &= ~SS_NOFDREF;	/* don't free the socket yet */
		if (smp){
			so->ref = 14;
	                smp_unlock(&so->lk_socket);
			smp_lock(&lk_tcb, LK_RETRY);
			smp_lock(&so->lk_socket, LK_RETRY);
			so->ref = 0;
			in_pcbdetach(inp);
			smp_unlock(&lk_tcb);
			so->so_state |= nofd;
		}else
			in_pcbdetach(inp);
		return (ENOBUFS);
	}
	tp->t_state = TCPS_CLOSED;
	return (0);
}

/*
 * Initiate (or continue) disconnect.
 * If embryonic state, just send reset (once).
 * If in ``let data drain'' option and linger null, just drop.
 * Otherwise (hard), mark socket disconnecting and drop
 * current input data; switch states based on user close, and
 * send segment to peer (with FIN).
 */
struct tcpcb *
tcp_disconnect(tp)
	register struct tcpcb *tp;
{
	struct socket *so = tp->t_inpcb->inp_socket;
	
	if (smp_debug){
		if (smp_owner(&so->lk_socket) == 0)
			panic ("tcp_disconnect not lock owner");
	}

	if (tp->t_state < TCPS_ESTABLISHED)
		tp = tcp_close(tp);
	else if ((so->so_options & SO_LINGER) && so->so_linger == 0)
		tp = tcp_drop(tp, 0);
	else {
		soisdisconnecting(so);
		sbflush(&so->so_rcv);
#ifdef XTI
		sbflush(&so->so_exrcv);
#endif XTI
		tp = tcp_usrclosed(tp);
		if (tp)
			(void) tcp_output(tp);
	}
	return (tp);
}

/*
 * User issued close, and wish to trail through shutdown states:
 * if never received SYN, just forget it.  If got a SYN from peer,
 * but haven't sent FIN, then go to FIN_WAIT_1 state to send peer a FIN.
 * If already got a FIN from peer, then almost done; go to LAST_ACK
 * state.  In all other cases, have already sent FIN to peer (e.g.
 * after PRU_SHUTDOWN), and just have to play tedious game waiting
 * for peer to send FIN or not respond to keep-alives, etc.
 * We can let the user exit from the close as soon as the FIN is acked.
 */
struct tcpcb *
tcp_usrclosed(tp)
	register struct tcpcb *tp;
{

	switch (tp->t_state) {

	case TCPS_CLOSED:
	case TCPS_LISTEN:
	case TCPS_SYN_SENT:
		tp->t_state = TCPS_CLOSED;
		tp = tcp_close(tp);
		break;

	case TCPS_SYN_RECEIVED:
	case TCPS_ESTABLISHED:
		tp->t_state = TCPS_FIN_WAIT_1;
		break;

	case TCPS_CLOSE_WAIT:
		tp->t_state = TCPS_LAST_ACK;
		break;
	}
	if (tp && tp->t_state >= TCPS_FIN_WAIT_2)
		soisdisconnected(tp->t_inpcb->inp_socket);
	return (tp);
}

