#ifndef lint
static char *sccsid = "@(#)bsc_timer.c	4.1		ULTRIX		 7/2/90";
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

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../net/net/if.h"
#include "../net/net/route.h"
#include "../net/netbsc/bsc.h"
#include "../net/netbsc/bsc_var.h"
#include "../net/netbsc/bsc_states.h"
#include "../net/netbsc/bsc_messages.h"
#include "../net/netbsc/bsc_timer.h"
#include "../h/protosw.h"
#include "../h/types.h"
#include "../h/ioctl.h"
#include "../h/ioctl.h"
#include "../h/errno.h"
#include "../h/cpudata.h"
#include "../h/proc.h"



/*
 * Fast timeout routine for processing delayed acks
 */
bsc_fasttimo()
{
	return(0);
}

/*
 * BSC protocol timeout routine called every 500 ms.
 * Causes finite state machine actions if timers expire.
 * Have noticed that on occasion, with the MVII, that the
 * IBM side will timeout before the 500 ms. period has expired.
 * Therefore change the 500 ms period to 400 ms.
 */
bsc_slowtimo()
{
	register struct bscpcb *bsc;
	register int i;
	int error = 0;
	extern short state;
	struct socket *so;
	struct bscpcb *bp;
	struct ifnet *ifp;
	struct sockaddr_bsc *sin;
	int saveaffinity;

	int s = splnet(); 
	/* saveaffinity = switch_affinity(boot_cpu_mask); */
	bsc = bsb.bscp_next; 
	
	/* We received a block of data and are not ready to process it.
	 * Send Wacks.
	 */
	if (state==SENDWACK) {
		bsc->b_timer++;
		sin = &bsc->bscp_laddr;
		ifp = (struct ifnet *)bsc_ifpfinder(sin);
		if (ifp == 0){
			so = bsc->bscp_socket;
			state = NULL;
			so->so_error = ETIMEDOUT;
			splx(s);
			return(0);
		}
		if (( bsc->b_timer == 40) && (bsc->b_rexmt<SEND_BLK_RETRY)){
			bsc->b_timer = 0;
			bsc->b_rexmt++;
			(void)xmtctl(WACK,NULL,ifp);
			splx(s);
			return(0);
		}
	}

	/* If we've sent an EOT we sleep expecting to get a message
	 * from the remote side.  If none comes in, then we wake the
	 * sleeping socket so we can either quit or send another file.
	 */
	if ( state == SENDACK0 ){
		bsc->b_timer++;
		sin = &bsc->bscp_laddr;
		ifp = (struct ifnet *)bsc_ifpfinder(sin);
		if (ifp == 0){
			so = bsc->bscp_socket;
			state = NULL;
			so->so_error = ETIMEDOUT;
			splx(s);
			return(0);
			}
		/* We are keeping the line alive while the application
		 * layer is trying to figure out who belongs to the file
		 * we just received.  Try to improve on this.	       */
		if (( bsc->b_timer == 50) && (bsc->b_rexmt<LINE_BID_RETRY)){
			bsc->b_timer = 0;
			bsc->b_rexmt++;
			(void)xmtctl(ENQ,NULL,ifp);
			splx(s);
			return(0);
			}
		if (bsc->b_rexmt >= LINE_BID_RETRY) {
			bsc->b_timer = 0;
			so = bsc->bscp_socket;
			state = NULL;
			so->so_error = ETIMEDOUT;
			splx(s);
			return(0);
			}
	}

	/* We are send a block and receive nothing.  We send an ENQ.
	 * If we get the ack we are expecting, then we send the next
	 * block of data.  If we got an ack, our state in the pcb is
	 * R_ACK so we transmit a TTD until the application layer is
	 * done preparing the next message to go out.
	 */
	if ((state==SENDBLK)||(state==SENDBLKLAST)) {
		bsc->b_timer++;
		sin = &bsc->bscp_laddr; 
		ifp = (struct ifnet *)bsc_ifpfinder(sin);
		if (ifp == 0){
			so = bsc->bscp_socket;
			state = NULL;
			so->so_error = ETIMEDOUT;
			splx(s);
			return(0);
		}
		/* We are sleeping on an RVI character which never came in
		 * so do a wakeup.		*/
		if (bsc->bscp_state == R_ACK){
			so = bsc->bscp_socket;
			wakeup((caddr_t)&so->so_timeo);
			splx(s);
			return(0);
		}
		if (( bsc->b_timer == 40) && (bsc->b_rexmt<SEND_BLK_RETRY)){
			bsc->b_timer = 0;
			bsc->b_rexmt++;
			if (bsc->bscp_state == NULL)
				(void)xmtctl(ENQ,NULL,ifp);
			if (bsc->bscp_state == R_ACK)  
				(void)xmtctl(STX,ENQ,ifp);
			splx(s);
			return(0);
		}
		if (bsc->b_rexmt >= SEND_BLK_RETRY) {
			state = NULL;
			bsc->b_timer = 0;
			bsc->b_rexmt = 0;
			bsc->bscp_state = NULL;  
			so = bsc->bscp_socket;
			/* Tnsmt EOT if you can't send a block because
			 * application layer will rebid for the line and send
			 * and ENQ and then expect to send signon card BUT
			 * the IBM side thinks the ENQ is a response to IBM
			 * not receiving the last block.  IBM doesn't take
			 * the ENQ as a fresh line bid.  EOT will tellIBM
			 * side that we've given up on the last message. */
			(void)xmtctl(EOT,NULL,ifp);
			so->so_error = ECONNREFUSED;  /* changed from
						       * ENETDOWN */
			wakeup((caddr_t)&so->so_timeo);
			splx(s);
		}
	}
	
	if( (state == INIT) ){
		bsc->b_timer++;
		if (( bsc->b_timer == 40) && (bsc->b_rexmt < LINE_BID_RETRY)){
			bsc->b_timer = 0;
			bsc->b_rexmt++;
			(void)bsc_output(bsc);
			splx(s);
			return(0);
			}
		if (bsc->b_rexmt >= LINE_BID_RETRY){
			bsc->b_rexmt = 0;
			state = NULL;
			soisconnected(bsc->bscp_socket);
			so = bsc->bscp_socket;
			so->so_error = ECONNABORTED;
			/* bsc = bsc_drop(bsc, ECONNABORTED); */
			splx(s);		
			return(0);
		}
	}
}

/*
 * Cancel timers.
 */
bsc_canceltimers(bsc)
	struct bscpcb *bsc;
{
	int saveaffinity;

	/* saveaffinity = switch_affinity(boot_cpu_mask); */
	bsc->b_timer = OFF;
	bsc->b_rexmt = 0;

}
/*
 * BSC timer processing.
 */
struct bscpcb *
bsc_timers(bsc)
	register struct bscpcb *bsc;

{	int error;
	struct mbuf m0;
	extern int BSC_READ_TIMER;
	extern int BSC_REXMT_TIMER;
	extern int MAX_RETRY;
	extern short state;
	struct bscpcb *bp;
	int saveaffinity;

	/* saveaffinity = switch_affinity(boot_cpu_mask); */
	if ((bsc->b_rexmt <= 7) && (state == INIT)){
			bsc->b_rexmt++;
			error = bsc_output(bsc);
			return(0);
	}
	if ((bsc->b_rexmt >= 7) && (state == INIT)){
			bsc->b_rexmt = 0;
			state = NULL;
			bp = bsc_drop(bsc, ECONNABORTED);
			return(0);
	}

	if ( (++bsc->b_rexmt == MAX_RETRY) && (BSC_REXMT_TIMER == 1)){
		printf( " in bsc_timers, timers have expired \n");
		error = ETIMEDOUT;
		bsc_canceltimers(bsc);
		bsc_close(bsc);
	}

	/*
	 * Retransmission timer went off.  Message has not
	 * been acked within retransmit interval. 
	 * Retransmit.
	 */
	if ( BSC_REXMT_TIMER == 1){
		switch(state){
		case INIT:
			(void)bsc_output(bsc);
			break;
		case SENDBLK:
			state = REXMT;
			(void)bsc_output(bsc);
			break;
		}
	}else{
	if (BSC_READ_TIMER == 1){
		error = ETIMEDOUT;
		bsc_canceltimers(bsc);
		bsc_close(bsc);
		}
	}

}

