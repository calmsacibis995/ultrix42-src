#ifndef lint
static char *sccsid = "@(#)udp_usrreq.c	4.7    (ULTRIX)        3/7/91";
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
 *      25-Feb-91	jsd
 *              account for all input packets, including error packets
 *		
 *      05-Nov-90    Mary Walker
 *              bzero sin_zero in udp_input (as a local it isn't zero'd)
 * 
 *	24-Jul-90	jaw
 *		fix locking problem in ip_input....
 *
 *	17-Jul-90	jsd
 *		remove so->ref panic since it's optimized out for single-cpu
 *
 *	7-Jul-90	lp
 *		FDDI performance.
 *
 *	06-Jun-90	R. Bhanukitsiri
 *		Initialize mbuf pointer to zero in udp_ctloutput().
 *
 *	26-Jan-89	gmm
 *		Changed declartion of inpcb pointer in udp_input() to volatile
 *
 *	2-Jan-89	U. Sinkewicz
 *		Performance enhancements to the uniprocessor kernel.
 *
 *	2-Dec-89	jsd
 *		Panic during so->ref check for single-cpu so we don't hang
 *
 *	12-Jun-89	R. Bhanukitsiri
 *		Add some of RFC 1066 MIB (network management).
 *
 *	30-May-89	U. Sinkewicz
 *		Added so->ref and SO_LOCK to fix smp problem caused
 *		by unlocking then locking socket to accommodate the
 *		lock hierarchy and sleeps.  Changed call interface to
 *		ip_output() - needed to support asymmetric network 
 *		drivers.		
 *
 *      05-May-89       Michael G. Mc Menemy
 *              Add XTI support.
 *	27-Mar-89	U. Sinkewicz
 *		Replaced udpstatistics with a macro as per lp changes
 *		from 3/16/89.
 *	3-Mar-89	U. Sinkewicz
 *		Pmax/smp merge.
 *	15-Jan-88	lp
 *		Merge of final 43BSD changes.
 *
 *	Chet Juszczak - 06/24/86					*
 *		Increase udp_recvspace to allow 8K NFS datagrams	*
 *									*
 *	Chet Juszczak - 03/12/86					*
 *		Increase maximum datagram size to 8K			*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	udp_usrreq.c	6.14 (Berkeley) 6/8/85
 */

#ifndef XTI
#define XTI
#endif
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/stat.h"
#include "../h/smp_lock.h"

#include "../net/net/if.h"
#include "../net/net/route.h"

#include "../net/netinet/in.h"
#include "../net/netinet/in_pcb.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"
#include "../net/netinet/ip_icmp.h"
#include "../net/netinet/udp.h"
#include "../net/netinet/udp_var.h"

/*
 * UDP protocol implementation.
 * Per RFC 768, August, 1980.
 */
struct lock_t lk_udb;	  /* SMP lock for udp */
struct lock_t lk_udpstat; /* SMP udpstat lock */
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
   * 0x80 = protocol;           \
   */                           \
  if (xti_debug & (level))      \
     cprintf((msg));
#else
#define PRINTXTID(level, msg)
#endif XTI

udp_init()
{
	int s;

	lockinit(&lk_udb, &lock_udb_d);
	lockinit(&lk_udpstat, &lock_udpstat_d);

	s = splnet();
	smp_lock(&lk_udb, LK_RETRY);
	udb.inp_next = udb.inp_prev = &udb;
	smp_unlock(&lk_udb);
	splx(s);
#ifdef XTI
	/* set-up XTI default characteristics */

	xti_udpinfo.addr = sizeof(struct sockaddr_in);
	xti_udpinfo.options = -2;
	xti_udpinfo.tsdu = 8192;  /* should be a constant */
	xti_udpinfo.etsdu = -2;
	xti_udpinfo.connect = -2;
	xti_udpinfo.discon = -2;
	xti_udpinfo.servtype = T_CLTS;
#endif XTI

}

int	udpcksum = 1;
int	udp_ttl = UDP_TTL;

udp_input(m0, ifp)
	struct mbuf *m0;
	struct ifnet *ifp;
{
	register struct udpiphdr *ui;
	register struct mbuf *m;
	register struct inpcb *inp_temp;
	int len;
	int s; 
	int mod_ui = 0;
	struct ip ip;
	struct	sockaddr_in udp_in; /* SMP */
	
	volatile struct inpcb *inp;

	udp_in.sin_family = AF_INET;
	bzero(udp_in.sin_zero, 8);

	/* account for all packets, even if we get an error later */
	UDPSTAT(udps_total++);
	UDPSTAT(udps_indatagrams++);

	/*
	 * Get IP and UDP header together in first mbuf.
	 */
	m = m0;
	if ((m->m_off > MMAXOFF || m->m_len < sizeof (struct udpiphdr)) &&
	    (m = m_pullup(m, sizeof (struct udpiphdr))) == 0) {
		UDPSTAT(udps_hdrops++);
		return;
	}
	ui = mtod(m, struct udpiphdr *);
	if (((struct ip *)ui)->ip_hl > (sizeof (struct ip) >> 2))
		ip_stripoptions((struct ip *)ui, (struct mbuf *)0);

	/*
	 * Make mbuf data length reflect UDP length.
	 * If not enough data to reflect UDP length, drop.
	 */
	len = ntohs((u_short)ui->ui_ulen);
	if (((struct ip *)ui)->ip_len != len) {
		if (len > ((struct ip *)ui)->ip_len) {
			UDPSTAT(udps_badlen++);
			goto bad;
		}
		m_adj(m, len - ((struct ip *)ui)->ip_len);
		/* (struct ip *)ui->ip_len = len; */
	}
	/*
	 * Checksum extended UDP header and data.
	 */
	if (udpcksum && ui->ui_sum) {

	/*
	 * Save copy of IP header in case we want to restore it for ICMP.
	 */
	ip = *(struct ip *)ui;

		ui->ui_next = ui->ui_prev = 0;
		ui->ui_x1 = 0;
		ui->ui_len = ui->ui_ulen;
		if (ui->ui_sum = in_cksum(m, len + sizeof (struct ip))) {
			UDPSTAT(udps_badsum++);
			m_freem(m);
			return;
		}
		mod_ui++;
	}

	/*
	 * Locate pcb for datagram.
	 */
	s = splnet(); /* SMP */
	smp_lock(&lk_udb, LK_RETRY);

	inp = in_pcblookup(&udb,
	    ui->ui_src, ui->ui_sport, ui->ui_dst, ui->ui_dport,
		INPLOOKUP_WILDCARD);
loop1:	if (inp == 0) {
		smp_unlock(&lk_udb);
		/* don't send ICMP response for broadcast packet */
		if (in_broadcast(ui->ui_dst)) {
			UDPSTAT(udps_noports++);
			goto bad;
		}
		if (mod_ui)
			*(struct ip *)ui = ip;
		icmp_error((struct ip *)ui, ICMP_UNREACH, ICMP_UNREACH_PORT,
			ifp);
		splx(s);
		UDPSTAT(udps_noports++);
		UDPSTAT(udps_nospace++);  /* count it as a dropped packet */
		return;
	}
	if (smp){
		smp_lock(&inp->inp_socket->lk_socket, LK_RETRY);
		while (inp->inp_socket->ref >0){
			smp_unlock(&inp->inp_socket->lk_socket);
			smp_unlock(&lk_udb);
			while ( (inp) && (inp->inp_socket->ref >0) )
				;
			smp_lock(&lk_udb, LK_RETRY);
			inp = in_pcblookup(&udb,
			    ui->ui_src,ui->ui_sport, ui->ui_dst, ui->ui_dport,
				INPLOOKUP_WILDCARD);
			if (inp == 0)
				goto loop1;
			smp_lock(&inp->inp_socket->lk_socket, LK_RETRY);
		}
		inp_temp = in_pcblookup(&udb,
		    ui->ui_src, ui->ui_sport, ui->ui_dst, ui->ui_dport,
			INPLOOKUP_WILDCARD);
		if (inp_temp != inp){
			smp_unlock(&inp->inp_socket->lk_socket);
			smp_unlock(&lk_udb);
			splx(s);
			UDPSTAT(udps_noports++);
			goto bad;
		}
	}

	smp_unlock(&lk_udb);		

	/*
	 * Construct sockaddr format source address.
	 * Stuff source address and datagram in user buffer.
	 */
	udp_in.sin_port = ui->ui_sport;
	udp_in.sin_addr = ui->ui_src;
	m->m_len -= sizeof (struct udpiphdr);
	m->m_off += sizeof (struct udpiphdr);
	if (sbappendaddr(&inp->inp_socket->so_rcv, (struct sockaddr *)&udp_in,
	    m, (struct mbuf *)0) == 0) {
		smp_unlock(&inp->inp_socket->lk_socket);
		splx(s);
		goto bad;
	}
#ifdef XTI
	if (((struct socket *) inp->inp_socket)->so_xticb.xti_evtenabled) {
	  ((struct socket *)inp->inp_socket)->so_xticb.xti_evtarray[XTI_EVT_T_DATA]++;
	  PRINTXTID(1,("T_DATA, len =%d,but 1 event generated\n",len - sizeof(struct udphdr)))
	}
#endif XTI
	sorwakeup(inp->inp_socket);
	smp_unlock(&inp->inp_socket->lk_socket);
	splx(s);
	return;
bad:
	UDPSTAT(udps_nospace++);  /* really number of "dropped" packets */
	m_freem(m);
}

/*
 * Notify a udp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
udp_notify(inp)
	register struct inpcb *inp;
{

	/* Ensure that socket is locked because
	 * routines that so*wakeup call write to the socket.
	 */
	/* Note that here, lk_udb is already held so if we use
	 * SO_LOCK, then SO_LOCK could so->ref busy, and we could 
	 * starve because lk_udb is held for the entire operation.
	 */
	smp_lock(&inp->inp_socket->lk_socket, LK_RETRY);
	sorwakeup(inp->inp_socket);
	sowwakeup(inp->inp_socket);
	smp_unlock(&inp->inp_socket->lk_socket);
}

udp_ctlinput(cmd, sa)
	int cmd;
	struct sockaddr *sa;
{
	extern u_char inetctlerrmap[];
	struct sockaddr_in *sin;
	int in_rtchange();

	if ((unsigned) cmd > PRC_NCMDS)
		return;
	if (sa->sa_family != AF_INET && sa->sa_family != AF_IMPLINK)
		return;
	sin = (struct sockaddr_in *)sa;
	if (sin->sin_addr.s_addr == INADDR_ANY)
		return;
	switch (cmd) {

	case PRC_QUENCH:
		break;

	case PRC_ROUTEDEAD:
	case PRC_REDIRECT_NET:
	case PRC_REDIRECT_HOST:
	case PRC_REDIRECT_TOSNET:
	case PRC_REDIRECT_TOSHOST:
		smp_lock(&lk_udb, LK_RETRY); 
		in_pcbnotify(&udb, &sin->sin_addr, 0, in_rtchange);
		smp_unlock(&lk_udb); 
		break;

	default:
		if (inetctlerrmap[cmd] == 0)
			return;		/* XXX */
notify:
		smp_lock(&lk_udb, LK_RETRY);
		in_pcbnotify(&udb, &sin->sin_addr, (int)inetctlerrmap[cmd], 
			udp_notify);
		smp_unlock(&lk_udb);
	}
}

udp_output(inp, m0)
	register struct inpcb *inp;
	struct mbuf *m0;
{
	register struct mbuf *m;
	register struct udpiphdr *ui;
	register int len = 0;
	int s;

	/*
	 * Calculate data length and get a mbuf
	 * for UDP and IP headers.
	 */
	for (m = m0; m; m = m->m_next)
		len += m->m_len;
	MGET(m, M_DONTWAIT, MT_DATA);
	if (m == 0) {
		m_freem(m0);
		return (ENOBUFS);
	}

	/*
	 * Fill in mbuf with extended UDP header
	 * and addresses and length put into network format.
	 */
	m->m_off = MMAXOFF - sizeof (struct udpiphdr);
	m->m_len = sizeof (struct udpiphdr);
	m->m_next = m0;
	ui = mtod(m, struct udpiphdr *);
	ui->ui_next = ui->ui_prev = 0;
	ui->ui_x1 = 0;
	ui->ui_pr = IPPROTO_UDP;
	ui->ui_len = htons((u_short)len + sizeof (struct udphdr));
	ui->ui_src = inp->inp_laddr;
	ui->ui_dst = inp->inp_faddr;
	ui->ui_sport = inp->inp_lport;
	ui->ui_dport = inp->inp_fport;
	ui->ui_ulen = ui->ui_len;

	/*
	 * Stuff checksum and output datagram.
	 */
	ui->ui_sum = 0;
	if (udpcksum) {
	    if ((ui->ui_sum = in_cksum(m, sizeof (struct udpiphdr) + len)) == 0)
		ui->ui_sum = -1;
	}
	((struct ip *)ui)->ip_len = sizeof (struct udpiphdr) + len;
	((struct ip *)ui)->ip_ttl = udp_ttl;
	UDPSTAT(udps_total++);
	UDPSTAT(udps_outdatagrams++);
	return (ip_output(m, inp->inp_options, &inp->inp_route,
	 inp->inp_socket->so_options & (SO_DONTROUTE | SO_BROADCAST), 
	 inp->inp_socket));
}

/* CJ - increased sendspace from 2K, recvspace from 4K */
int	udp_sendspace = 9000;		/* really max datagram size */
int	udp_recvspace = 9000;


/*ARGSUSED*/
udp_usrreq(so, req, m, nam, rights)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	struct inpcb *inp = sotoinpcb(so);
	int error = 0;
	struct socket *so_tmp;		/* SMP */
	struct socket *so_addr = NULL;	/* SMP */
	int status;			/* SMP */
	struct inpcb *inp_tmp = NULL; 	/* SMP */
	struct inpcb *head = NULL; 	/* SMP */

	/*
	 * SMP: Socket lock set coming in all cases save SENSE.  
	 * Up to splnet coming in. 
	 */

	if (req == PRU_CONTROL)
		return (in_control(so, (int)m, (caddr_t)nam,
			(struct ifnet *)rights));
	if (rights && rights->m_len) {
		error = EINVAL;
		goto release;
	}
	if (inp == NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}
	switch (req) {

	case PRU_ATTACH:
		if (inp != NULL) {
			error = EINVAL;
			break;
		}
		if (smp){
			so->ref = 15;
	                smp_unlock(&so->lk_socket);
			smp_lock(&lk_udb, LK_RETRY);
			smp_lock(&so->lk_socket, LK_RETRY);
			so->ref = 0;
			error = in_pcballoc(so, &udb);
			smp_unlock(&lk_udb);
		}else
			error = in_pcballoc(so, &udb);
		if (error)
			break;
		error = soreserve(so, udp_sendspace, udp_recvspace);
		if (error)
			break;
#ifdef XTI
		so->so_xticb.xti_tpinfo = &xti_udpinfo;
#endif XTI
		break;

	case PRU_DETACH:
		if (inp == NULL) {
			error = ENOTCONN;
			break;
		}
		if (smp){
			so->ref = 16;
	                smp_unlock(&so->lk_socket);
			smp_lock(&lk_udb, LK_RETRY);
			smp_lock(&so->lk_socket, LK_RETRY);
			so->ref = 0;
			in_pcbdetach(inp);
			smp_unlock(&lk_udb);
		}else
			in_pcbdetach(inp);
		break;

	case PRU_BIND:
		if (smp){
			so->ref = 17;
	                smp_unlock(&so->lk_socket);
			smp_lock(&lk_udb, LK_RETRY);
			smp_lock(&so->lk_socket, LK_RETRY);
			so->ref = 0;
			error = in_pcbbind(inp, nam);
			smp_unlock(&lk_udb);
		}else
			error = in_pcbbind(inp, nam);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			error = EISCONN;
			break;
		}
		if (smp){
			so->ref = 18;
	                smp_unlock(&so->lk_socket);
			smp_lock(&lk_udb, LK_RETRY);
			smp_lock(&so->lk_socket, LK_RETRY);
			so->ref = 0;
			error = in_pcbconnect(inp, nam);
			smp_unlock(&lk_udb);
		}else
			error = in_pcbconnect(inp, nam);
		if (error == 0)
			soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_ACCEPT:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
			error = ENOTCONN;
			break;
		}
		if (smp){
			so->ref = 19;
	                smp_unlock(&so->lk_socket);
			smp_lock(&lk_udb, LK_RETRY);
			smp_lock(&so->lk_socket, LK_RETRY);
			so->ref = 0;
			in_pcbdisconnect(inp);
			smp_unlock(&lk_udb);
		}else
			in_pcbdisconnect(inp);
		so->so_state &= ~SS_ISCONNECTED;		/* XXX */
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_SEND: {
		struct in_addr laddr;

		if (nam) {
			laddr = inp->inp_laddr;
			if (inp->inp_faddr.s_addr != INADDR_ANY) {
				error = EISCONN;
				break;
			}
			/*
			 * Must block input while temporarily connected.
			 */
			if (smp){
				so->ref = 20;
		                smp_unlock(&so->lk_socket);
				smp_lock(&lk_udb, LK_RETRY);
				smp_lock(&so->lk_socket, LK_RETRY);
				so->ref = 0;
				error = in_pcbconnect(inp, nam);
				smp_unlock(&lk_udb);
			}else
				error = in_pcbconnect(inp, nam);
			if (error) {
				break;
			}
		} else {
			if (inp->inp_faddr.s_addr == INADDR_ANY) {
				error = ENOTCONN;
				break;
			}
		}
		error = udp_output(inp, m);
		m = NULL;
		if (nam) {
			if (smp){
				so->ref = 21;
		                smp_unlock(&so->lk_socket);
				smp_lock(&lk_udb, LK_RETRY);
				smp_lock(&so->lk_socket, LK_RETRY);
				so->ref = 0;
				in_pcbdisconnect(inp);
				smp_unlock(&lk_udb);
			}else
				in_pcbdisconnect(inp);
			inp->inp_laddr = laddr;
		}
		}
		break;

	case PRU_ABORT:
		soisdisconnected(so);
		if (smp){
			so->ref = 22;
	                smp_unlock(&so->lk_socket);
			smp_lock(&lk_udb, LK_RETRY);
			smp_lock(&so->lk_socket, LK_RETRY);
			so->ref = 0;
			in_pcbdetach(inp);
			smp_unlock(&lk_udb);
		}else
			in_pcbdetach(inp);
		break;

	case PRU_SOCKADDR:
		in_setsockaddr(inp, nam);
		break;

	case PRU_PEERADDR:
		in_setpeeraddr(inp, nam);
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		return (0);

	case PRU_SENDOOB:
	case PRU_FASTTIMO:
	case PRU_SLOWTIMO:
	case PRU_PROTORCV:
	case PRU_PROTOSEND:
		error =  EOPNOTSUPP;
		break;

	case PRU_RCVD:
	case PRU_RCVOOB:
		return (EOPNOTSUPP);	/* do not free mbuf's */

	default:
		error = EOPNOTSUPP;
	}
release:
	if (m != NULL)
		m_freem(m);
	return (error);
}

#ifdef XTI

udp_ctloutput(op, so, level, optname, mp)
        int op;
        struct socket *so;
        int level, optname;
        struct mbuf **mp;
{


        int error = 0;
	struct inpcb *inp = sotoinpcb(so);
	register struct mbuf *m = 0;
  
	if (level != IPPROTO_UDP)
	  return (ip_ctloutput(op, so, level, optname, mp));

	switch (op) {

	case PRCO_XTIMAPSTATE:
	  so->so_xticb.xti_states = udp_to_xtistate(so);
	  break;
    
	case PRCO_XTIMAPINFO:
	  so->so_xticb.xti_tpinfo = 
	    &xti_udpinfo; /* info is per provider */
	  break;

	case PRCO_XTIUNBIND:

	  xtiin_pcbunbind(so->so_pcb);
	  break;

	case PRCO_SETOPT:
	  m = *mp;
	  switch (optname) {
	    
	  default:
	    error = EINVAL;
	    break;
	  };
	  break;

	case PRCO_GETOPT:
	  {
	    int tmp_status;
      
	    *mp = m = m_get(M_WAIT, MT_SOOPTS);
	    m->m_len = sizeof(int);
    
	    switch (optname) {
	
	    default:
	      error = EINVAL;
	      break;
	    };
	    break;
	  }
	};
	if(m)
	  (void) m_free(m);
	return (error);
}


/*
 * KEY
 * ---
 * so   - socket pointer
 * inp  - internet control block pointer
 * fd   - file desciptor
 *
 * STATE        MINIMUM MAPPING
 * ----------   -----------------------------
 * T_UNINIT     (getsock(fd) == 0))
 * T_UNBND      (getsock(fd) > 0)
 * T_IDLE	(inp->inp_lport || inp->inp_fport)
 */

udp_to_xtistate(so)
        struct socket *so;
{

        int status;
	int default_state;
	struct inpcb *inp;

	/*
	 * determine what state we are in
	 */

	/*
	 * we are already past T_UNINIT state, because
	 * if we were a bogus descriptor or a non-socket descriptor we
	 * would have been caught.
	 */

	default_state = T_UNBND;
	
	if (so) inp = (struct inpcb *) so->so_pcb;
	
	if (inp)
	  if (inp->inp_lport || inp->inp_fport) {
	    default_state = T_IDLE;
	  } else {
	    return(default_state);
	  }
	else
	  return(default_state);
	
	return(default_state);
}

#endif XTI
