#ifndef lint
static	char	*sccsid = "@(#)tcp_timer.c	4.3		(ULTRIX)		4/30/91";
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
 *	28-Apr-91	jsd, woodward
 *		Add debug code to see if udb lock is being lost
 *
 *	3-June-89	Ursula Sinkewicz
 *		Added checks so->ref.  This means that if a socket is
 *		unlocked, say for the lock heirarchy, then the timers 
 *		bypass that socket.
 *
 *	27-Mar-89	Ursula Sinkewicz
 *		Replaces tcpstatistics with a macro as per lp changes
 *		made on 3/16/89/
 *
 *	3-Mar-89	Ursula Sinkewicz
 *		Pmax/smp merge. 
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
 *	tcp_timer.c	6.7 (Berkeley) 6/8/85
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/smp_lock.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
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

int	tcpnodelack = 0;
/*
 * Fast timeout routine for processing delayed acks
 */
/* new 7/1/89: tcp_fasttimo no longer goes through the tcb chain
 * when it finds a DELACK.  Now it acts on the first DELACK and
 * returns (early).  This avoids the overhead of unlocking and
 * re-locking the lk_tcb, as well as worrying whether the inp->inp_next
 * is valid.  We probably get a DELACK once a second, and if there
 * are two DELACKs in the chain, we will just pick up the second
 * in another 200ms.
 */
tcp_fasttimo()
{
	register struct inpcb *inp, *inpnxt;
	register struct tcpcb *tp;
	int s = splnet();
	struct socket *so;

	smp_lock(&lk_tcb, LK_RETRY);
	inp = tcb.inp_next;
	if (inp)
	for (; inp != &tcb; inp = inp->inp_next) {
	    if ((tp = (struct tcpcb *)inp->inp_ppcb) &&
			(tp->t_flags & TF_DELACK)) {
		so = tp->t_inpcb->inp_socket;
		if ((smp_lock(&so->lk_socket, LK_ONCE)) == 1) {
			if (so->ref > 0 ){
				smp_unlock(&so->lk_socket);
				continue;
			}
			tp->t_flags &= ~TF_DELACK;
			tp->t_flags |= TF_ACKNOW;
			TCPSTAT(tcps_delack++);
			smp_unlock(&lk_tcb);
			(void) tcp_output(tp);
			smp_unlock(&so->lk_socket);
			goto quit;
		}
	    }
	}
	smp_unlock(&lk_tcb);
quit:
	splx(s);
	return;
}
int tcp_slow_active;  /* is 1 if currently in tcp_slowtimo() */
/*
 * Tcp protocol timeout routine called every 500 ms.
 * Updates the timers in all active tcb's and
 * causes finite state machine actions if timers expire.
 */
tcp_slowtimo()
{
	register struct inpcb *ip, *ipnxt;
	register struct tcpcb *tp;
	register struct socket *so;
	int s = splnet();
	register int i;
	struct socket *so_temp;
	int foob;	/* DEBUG */

	smp_lock(&lk_tcpiss, LK_RETRY);
	tcp_slow_active = 0;
	smp_unlock(&lk_tcpiss);

	/*
	 * Search through tcb's and update active timers.
	 */
	smp_lock(&lk_tcb, LK_RETRY);
	ip = tcb.inp_next;
	if (ip == 0) {
		smp_unlock(&lk_tcb);
		splx(s);
		return;
	}
	smp_lock(&lk_tcpiss, LK_RETRY);
	tcp_slow_active = 1;
	smp_unlock(&lk_tcpiss);

	for (; ip != &tcb; ip = ipnxt) {
		foob = lk_tcb.l_won;	/* DEBUG */
		ipnxt = ip->inp_next;
		tp = intotcpcb(ip);
		if (tp == 0){
			continue;
		}
		for (i = 0; i < TCPT_NTIMERS; i++) {
		    if ( tp->t_timer[i] > 0 ) {
			so = tp->t_inpcb->inp_socket;
			/* 'Deadly Embrace' is possible with locking in
			 * so and so_head and tcb in other routines.  
			 * Alleviate the situation here
		 	 * in the timers, by looking to see if the socket
			 * is locked or ref'ed and if it is, go on.
			 */
			if ( (smp_lock(&so->lk_socket, LK_ONCE)) == 0 )
				goto tpgone;
		        if ( (tp->t_timer[i] && --tp->t_timer[i] == 0) == 0){
				smp_unlock(&so->lk_socket);
				continue;
			}
			if (so->ref > 0 ){
				smp_unlock(&so->lk_socket);
				goto tpgone;
			}
			so_temp = so;

			(void) tcp_usrreq(tp->t_inpcb->inp_socket,
				PRU_SLOWTIMO, (struct mbuf *)0,
				(struct mbuf *)i, (struct mbuf *)0);
			/* handle case of tcp_close called from tcp_timer */
			if (smp_owner(&so_temp->lk_socket) == 1)
				smp_unlock(&so->lk_socket); 
			if (ipnxt->inp_prev != ip)
		   		goto tpgone;

		    }
		}
		tp->t_idle++;
		if (tp->t_rtt)
			tp->t_rtt++;
tpgone:
		if (foob != lk_tcb.l_won) {	/* DEBUG */
			mprintf("tcp_slowtimo: lost tcb lock unexpectedly\n");
			goto tpexit;
		}

	}
tpexit:
	smp_unlock(&lk_tcb);
	smp_lock(&lk_tcpiss, LK_RETRY);
	tcp_iss += TCP_ISSINCR/PR_SLOWHZ;		/* increment iss */
	if((int)tcp_iss < 0) /* 4.2 compatibility */
		tcp_iss = 0;
	tcp_slow_active = 0;
	smp_unlock(&lk_tcpiss);
	splx(s);
}

/*
 * Cancel all timers for TCP tp.
 */
tcp_canceltimers(tp)
	struct tcpcb *tp;
{
	register int i;

	for (i = 0; i < TCPT_NTIMERS; i++)
		tp->t_timer[i] = 0;
}

int tcp_backoff[TCP_MAXRXTSHIFT+1] =
    { 1, 2, 4, 8, 16, 32, 64, 64, 64, 64, 64, 64, 64 };
/*
 * TCP timer processing.
 */
/*
 * SMP: Socket is locked coming in.  Lock set in tcp_slowtimeo.
 */
struct tcpcb *
tcp_timers(tp, timer)
	register struct tcpcb *tp;
	int timer;
{
	register int rexmt;
	struct socket *so = tp->t_inpcb->inp_socket;
	int s;

	if (smp_debug){
		if (smp_owner(&so->lk_socket) == 0)
			panic("tcp_timers not lock owner");
	}
	switch (timer) {
	/*
	 * 2 MSL timeout in shutdown went off.  If we're closed but
	 * still waiting for peer to close and connection has been idle
	 * too long, or if 2MSL time is up from TIME_WAIT, delete connection
	 * control block.  Otherwise, check again in a bit.
	 */
	case TCPT_2MSL:
		if (tp->t_state != TCPS_TIME_WAIT &&
		    tp->t_idle <= TCPTV_MAXIDLE)
			tp->t_timer[TCPT_2MSL] = TCPTV_KEEP;
		else {
			tp = tcp_close(tp);
		}
		break;

	/*
	 * Retransmission timer went off.  Message has not
	 * been acked within retransmit interval.  Back off
	 * to a longer retransmit interval and retransmit one segment.
	 */
	case TCPT_REXMT:
		if (++tp->t_rxtshift > TCP_MAXRXTSHIFT) {
			tp->t_rxtshift = TCP_MAXRXTSHIFT;
			TCPSTAT(tcps_timeoutdrop++);
			tp = tcp_drop(tp, ETIMEDOUT);
			break;
		}
		TCPSTAT(tcps_rexmttimeo++);
                rexmt = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;
		rexmt *= tcp_backoff[tp->t_rxtshift];
                TCPT_RANGESET(tp->t_rxtcur, rexmt, TCPTV_MIN, TCPTV_REXMTMAX);
                tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
		/*
                 * If losing, let the lower level know and try for
		 * a better route.  Also, if we backed off this far,
		 * our srtt estimate is probably bogus.  Clobber it
                 * so we'll take the next rtt measurement as our srtt;
		 * move the current srtt into rttvar to keep the current
                 * retransmit times until then.
		 */
		if (tp->t_rxtshift >= TCP_MAXRXTSHIFT / 4) {
			in_losing(tp->t_inpcb);
                        tp->t_rttvar += (tp->t_srtt >> 2);
			tp->t_srtt = 0;
		}
		tp->snd_nxt = tp->snd_una;
		/*
		 * If timing a segment in this window stop the timer.
		 */
		tp->t_rtt = 0;
		/*
		 * Close the congestion window down to one segment
		 * (we'll open it by one segment for each ack we get).
		 * Since we probably have a window's worth of unacked
		 * data accumulated, this "slow start" keeps us from
		 * dumping all that data as back-to-back packets (which
		 * might overwhelm an intermediate gateway).
		 *
		 * There are two phases to the opening: Initially we
		 * open by one mss on each ack.  This makes the window
		 * size increase exponentially with time.  If the
		 * window is larger than the path can handle, this
		 * exponential growth results in dropped packet(s)
		 * almost immediately.  To get more time between 
		 * drops but still "push" the network to take advantage
		 * of improving conditions, we switch from exponential
		 * to linear window opening at some threshhold size.
		 * For a threshhold, we use half the current window
		 * size, truncated to a multiple of the mss.
		 *
		 * (the minimum cwnd that will give us exponential
		 * growth is 2 mss.  We don't allow the threshhold
		 * to go below this.)
		 */
		{
		u_int win = MIN(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;
		if (win < 2)
			win = 2;
		tp->snd_cwnd = tp->t_maxseg;
		tp->snd_ssthresh = win * tp->t_maxseg;
		}
		(void) tcp_output(tp);
		break;

	/*
	 * Persistance timer into zero window.
	 * Force a byte to be output, if possible.
	 */
	case TCPT_PERSIST:
                TCPSTAT(tcps_persisttimeo++);
		tcp_setpersist(tp);
		tp->t_force = 1;
		(void) tcp_output(tp);
		tp->t_force = 0;
		break;

	/*
	 * Keep-alive timer went off; send something
	 * or drop connection if idle for too long.
	 */
	case TCPT_KEEP:
		TCPSTAT(tcps_keeptimeo++);
		if (tp->t_state < TCPS_ESTABLISHED)
			goto dropit;
		if (tp->t_inpcb->inp_socket->so_options & SO_KEEPALIVE &&
			tp->t_state <= TCPS_CLOSE_WAIT) {
		    	if (tp->t_idle >= TCPTV_MAXIDLE)
				goto dropit;
			/*
			 * Send a packet designed to force a response
			 * if the peer is up and reachable:
			 * either an ACK if the connection is still alive,
			 * or an RST if the peer has closed the connection
			 * due to timeout or reboot.
			 * Using sequence number tp->snd_una-1
			 * causes the transmitted zero-length segment
			 * to lie outside the receive window;
			 * by the protocol spec, this requires the
			 * correspondent TCP to respond.
			 */
			TCPSTAT(tcps_keepprobe++);
#ifndef TCP_43BSD
			/*
			 * The keepalive packet must have nonzero length
			 * to get a 4.2 host to respond.
			 */
			tcp_respond(tp, tp->t_template,
			    tp->rcv_nxt - 1, tp->snd_una - 1, 0, so);
#else
			tcp_respond(tp, tp->t_template,
			    tp->rcv_nxt, tp->snd_una - 1, 0, so);
#endif
		} 
		tp->t_timer[TCPT_KEEP] = TCPTV_KEEP;
		break;
	dropit:
                TCPSTAT(tcps_keepdrops++);
		tp = tcp_drop(tp, ETIMEDOUT);
		break;
	}
	return (tp);
}


