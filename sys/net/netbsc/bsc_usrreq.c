#ifndef lint
static char *sccsid = "@(#)bsc_usrreq.c	4.1	ULTRIX		7/2/90";
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
/*	bsc_usrreq.c	1.0	3/20/85				     */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/errno.h"
#include "../h/types.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/cpudata.h"
#include "../h/proc.h"

#include "../net/netinet/in_systm.h"

#include "../net/net/if.h"
#include "../net/net/route.h"

#include "../net/netbsc/bsc_states.h"
#include "../net/netbsc/bsc.h"
#include "../net/netbsc/bsc_var.h"
#include "../net/netbsc/bsc_messages.h"
#include "../net/netbsc/bsc_timer.h"

/*
 * BSC protocol interface to socket abstraction.
 */
/* extern	char *bscstates[];	*/
int	bscsenderrors;
extern timeout();


/*
 * Process a BSC user request.  If it is a send request
 * then m is the mbuf chain of send data.  
 */

/*ARGSUSED*/
bsc_usrreq(so, req, m, nam, rights)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
{
	register struct bscpcb *bsc = sotobscpcb(so);
	struct bsc_data *bscdat;
	int s = splnet();
	int error = 0;
	int saveaffinity;

	saveaffinity = switch_affinity(boot_cpu_mask);

	/* 
	 * The following if statement added to comply with subnet routing
	 * changes added 9/19/85.  Note that nam here is really data, eg
	 * address.  Also note that ifp was located
	 * by searching ifps for a match on if_name field, supplied 
	 * by user command when bringing up the network.  Here, ifp
	 * is really "dup" or whatever and is referred to in the bsc_control
	 * call as rights which is misleading because it means something
	 * else for every PRU_xxx case except PRU_CONTROL.
	 */
	if (req == PRU_CONTROL)
	return( bsc_control(so, (int)m, (caddr_t)nam,(struct ifnet *)rights));

	if (rights && rights->m_len) {
		splx(s);
		return (EINVAL);
	}
	/*
	 * Do a repeatable sanity check on the pcb.
	 */
	if (bsc == 0 && req != PRU_ATTACH) {
		splx(s);
		return (EINVAL);
	}


	switch (req) {
	/*
	 * BSC attaches to socket via PRU_ATTACH, reserving space,
	 * and a control block.
	 */
	case PRU_ATTACH:
		/* You may want to use the protocol field in the socket call
		 * to identify which bisync protocol you are using.
		 */
		if (bsc) {
			error = EISCONN;
			break;
		}
		error = bsc_attach(so);
		if (error)
			break;
		if ((so->so_options & SO_LINGER) && so->so_linger == 0)
			so->so_linger = BSC_LINGERTIME;
		break;

	/*
	 * PRU_DETACH detaches the BSC protocol from the socket.
	 * be discarded here.
	 */
	case PRU_DETACH:
		
		state = CLOSED;  
		so->so_pcb = 0;
		sofree(so);
		remque(bsc);
		KM_FREE(bsc, MT_PCB);
		break;

	/*
	 * Give the socket an address so that it can later be used to
	 * identify an interface.
	 */
	case PRU_BIND:
		error = bsc_pcbbind(bsc, nam);
		if (error)
			break;
		break;

	/*
	 * Prepare to accept connections.
	 */
	case PRU_LISTEN:
		break;

	/*
	 * Initiate connection to peer.  Initialize timers and retransmit
	 * flags.  Enter INIT state, and mark socket as connecting.
	 * MODEM state is not used here.  MODEM is intended for use with
	 * synchronous aurodial.  All that is needed to acitvate auto
	 * dial code, is to uncomment the code here in PRU_CONNECT.
	 * The timers and protocol state stuff are all prepared to 
	 * respond to state MODEM so they do not need to be changed.
	 */
	case PRU_CONNECT:{

	/*	bsc->bscp_state = MODEM; 
	 * 	bsc->b_timer = 0;
 	 *	state = MODEM;
	 * 	error = bsc_pcbconnect(bsc, nam);
	 *	if (error) 
	 *		break;
	 *	error = bsc_output(bsc);
	 *	if (error)
	 *		break;
	 *	sosiconnecting(s0);
	 */
	
		bsc->bscp_state = INIT;
		bsc->b_timer = 0;
		bsc->b_rexmt = 0;
		state = INIT;

		error = bsc_pcbconnect(bsc, nam);
		if (error) 
			break;

		soisconnecting(so);

		error = bsc_output(bsc);
		break;
	}
	 
	/*
	 * Send some data.
	 */
	case PRU_SEND:{

		sbappend(&so->so_snd, m);

		state = SENDBLK;  /* Took this line out of
				   * SENDACK0 - R_ACK and put it here */
		bsc->bscp_state = NULL;
		bsc->b_iobc = 0; 
		bsc->b_timer = 0;
		bsc->b_rexmt = 0;
		BSC_REXMT_TIMER = 0;
		BSC_READ_TIMER = 0;

		error = bsc_output(bsc);
		if ( error )
			break;

		sleep((caddr_t)&so->so_timeo, PZERO+1);
		saveaffinity = switch_affinity(boot_cpu_mask);
		error = so->so_error;
		if ( error )
			break;

		break;
		}

	case PRU_RCVD:
		bsc->b_iobc = 0;	
		state = SENDACK;
		BSC_READ_TIMER = 1;
		bsc->b_timer = BSC_DATA_TIMER;
		break;

	case PRU_RCVOOB:
	/* Sleep here and wakeup when data comes in so that when the
	 * read starts, there will already be something there there
	 * to read.  Otherwise, the socket will unblock the rcv queue
	 * and return without doing anything.
	 */
	sleep((caddr_t)&so->so_timeo, PZERO+1);  
	saveaffinity = switch_affinity(boot_cpu_mask);
	error = so->so_error;
	if (error)
		break;

	if (bsc->b_iobc != 0){
		m->m_len = 1;
		/* *mtod(m, caddr_t) = bsc->b_iobc; */
		bscdat = mtod(m, struct bsc_data *);
		bscdat->data[0] = bsc->b_iobc;
		bsc->b_iobc = 0;
	}else{
		m->m_len = 1;
		bscdat = mtod(m, struct bsc_data *);
		bscdat->data[0] = 0;
		bsc->b_iobc = 0;
	}
	break;

	case PRU_SLOWTIMO:{

		break;
	}
	/*
	 * Initiate disconnect from peer.
	 */
	case PRU_DISCONNECT:
		state = CLOSED;  
		so->so_pcb = 0;
		sofree(so);
		remque(bsc);
		KM_FREE(bsc, MT_PCB);
		
		soisdisconnected(so);
		break;

	case PRU_ACCEPT: {
		break;
		}

	/*
	 * Mark the connection as being incapable of further output.
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		bsc= bsc_usrclosed(bsc);
		break;


	/*
	 * Abort the BSC.
	 */
	case PRU_ABORT:{

		struct bscpcb *bp;

		bp= bsc_drop(bp, ECONNABORTED);
		break;
		}

	default:
		error = EOPNOTSUPP;
		panic("bsc_usrreq");
		break;
	}

	splx(s);
	return (error);
}


