/*	tcp_debug.c	6.1	83/07/29	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/smp_lock.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#define PRUREQUESTS
#include "../h/protosw.h"
#include "../h/errno.h"

#include "../net/net/route.h"
#include "../net/net/if.h"

#include "../net/netinet/in.h"
#include "../net/netinet/in_pcb.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"
#include "../net/netinet/tcp.h"
#define TCPSTATES
#include "../net/netinet/tcp_fsm.h"
#include "../net/netinet/tcp_seq.h"
#define	TCPTIMERS
#include "../net/netinet/tcp_timer.h"
#include "../net/netinet/tcp_var.h"
#include "../net/netinet/tcpip.h"
#define	TANAMES
#include "../net/netinet/tcp_debug.h"

int	tcpconsdebug = 0;
/*
 * Tcp debug routines
 */
tcp_trace(act, ostate, tp, ti, req)
	short act, ostate;
	struct tcpcb *tp;
	struct tcpiphdr *ti;
	int req;
{
	tcp_seq seq, ack;
	int len, flags;
	int s;
	struct tcp_debug *td;

	s = splimp();
	smp_lock(&lk_net_mgt, LK_RETRY);

	td = &tcp_debug[tcp_debx++];
	if (tcp_debx == TCP_NDEBUG)
		tcp_debx = 0;
	td->td_time = iptime();
	td->td_act = act;
	td->td_ostate = ostate;
	td->td_tcb = (caddr_t)tp;
	if (tp)
		td->td_cb = *tp;
	else
		bzero((caddr_t)&td->td_cb, sizeof (*tp));
	if (ti)
		td->td_ti = *ti;
	else
		bzero((caddr_t)&td->td_ti, sizeof (*ti));
	td->td_req = req;
	if (tcpconsdebug == 0){
		smp_unlock(&lk_net_mgt);
		splx(s);
		return;
	}
	if (tp)
		printf("%x %s:", tp, tcpstates[ostate]);
	else
		printf("???????? ");
	printf("%s ", tanames[act]);
	switch (act) {

	case TA_INPUT:
	case TA_OUTPUT:
	case TA_DROP:
		if (ti == 0)
			break;
		seq = ti->ti_seq;
		ack = ti->ti_ack;
		len = ti->ti_len;
		if (act == TA_OUTPUT) {
			seq = ntohl(seq);
			ack = ntohl(ack);
			len = ntohs((u_short)len);
		}
		if (act == TA_OUTPUT)
			len -= sizeof (struct tcphdr);
		if (len)
			printf("[%x..%x)", seq, seq+len);
		else
			printf("%x", seq);
		printf("@%x, urp=%x", ack, ti->ti_urp);
		flags = ti->ti_flags;
		if (flags) {
#ifndef lint
			char *cp = "<";
#define pf(f) { if (ti->ti_flags&TH_/**/f) { printf("%s%s", cp, "f"); cp = ","; } }
			pf(SYN); pf(ACK); pf(FIN); pf(RST); pf(PUSH); pf(URG);
#endif
			printf(">");
		}
		break;

	case TA_USER:
		printf("%s", prurequests[req&0xff]);
		if ((req & 0xff) == PRU_SLOWTIMO)
			printf("<%s>", tcptimers[req>>8]);
		break;
	}
	if (tp)
		printf(" -> %s", tcpstates[tp->t_state]);
	/* print out internal state of tp !?! */
	printf("\n");
	if (tp == 0){
		smp_unlock(&lk_net_mgt);
		splx(s);
		return;
	}
	printf("\trcv_(nxt,wnd,up) (%x,%x,%x) snd_(una,nxt,max) (%x,%x,%x)\n",
	    tp->rcv_nxt, tp->rcv_wnd, tp->rcv_up, tp->snd_una, tp->snd_nxt,
	    tp->snd_max);
	printf("\tsnd_(wl1,wl2,wnd) (%x,%x,%x)\n",
	    tp->snd_wl1, tp->snd_wl2, tp->snd_wnd);

	smp_unlock(&lk_net_mgt);
	splx(s);

}
