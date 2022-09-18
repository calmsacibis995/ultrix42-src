#ifndef lint
static char *sccsid = "@(#)lat_subr.c	4.1.1.3	2/29/88";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
 *	Peter Harbo - 4/15/86						*
 * 		New rxmit_ackq() to modify transport mechanism for      *
 *		DECnet-DOS.
 ************************************************************************/


/*	lat_subr.c	0.0	11/9/84	*/
/*      lat_subr.c      1.0     4/15/86 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"

#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../lat/lat.h"
#include "../lat/lat_protocol.h"
#include "../lat/lat_var.h"

extern u_char lattmoactive;
extern struct lat_counters latctrs;
extern struct socket *lat_traceso;

/*
 * LAT common support subroutines.
 */

/*
 *		l a t s e n d
 *
 * Attempt to transmit all pending messages for the specified virtual circuit.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir		= Pointer to virtual circuit descriptor.
 */
latsend( vcir )
register struct lat_vc *vcir;
{
    register struct mbuf *m;
    register struct vc_hdr *hdr;

    while (vcir->lvc_xmtq.q_head)
    {
	if (m = m_copy(vcir->lvc_xmtq.q_head, 0, M_COPYALL))
	{
	    hdr = mtod(m, struct vc_hdr *);
	    hdr->vhd_ack = vcir->lvc_ack;
	    if (lat_traceso)
		ltt_trace(0,0,m,(vcir->lvc_addr).lat_addr);
	    (*vcir->lvc_if->if_output)(vcir->lvc_if, m, &vcir->lvc_addr);
	    DEQUEUE(&vcir->lvc_xmtq, m);
	    ENQUEUE(&vcir->lvc_ackq, m);
	    vcir->lvc_timer = (vcir->lvc_rrf || hdr->vhd_type == MSG_START)
	        ? LAT_XTIMER : 3*(vcir->lvc_kalive*2);
	    if (lattmoactive == 0)
		vcir->lvc_counter = LAT_XRETRY;
            if (lat_master(vcir)) 	/* latmaster */
	    {
	        vcir->lvc_timer =  LAT_XTIMER;
                vcir->lvc_dgsize = 0; 
	    }
	}
	else
	{
	    break;
	}
    }
}

/*
 *			r x m i t _ a c k q
 *
 * Retransmit the pending acknowledge queue.
 *
 * Outputs:		None.
 *
 * Inputs:
 *	vcir	=	Virtual circuit whose pending acknowledge queue
 *				is to be retransmitted.
 *		
 */
rxmit_ackq(vcir)
struct lat_vc *vcir;
{
	register struct mbuf *m;

	if (vcir->lvc_counter--)
	{
		/*
		 * Move the pending ack queue to the front of the
		 * pending transmit queue and restart message
		 * transmission.
		 */
		if (m = vcir->lvc_ackq.q_head)
		{
		/*
		 * Count retransmissions.
		 */
			do {
				INC(lco_rexmt);
			} while (m = m->m_act);

			if (vcir->lvc_xmtq.q_head)
			{
				vcir->lvc_ackq.q_tail->m_act = 
					vcir->lvc_xmtq.q_head;
				vcir->lvc_ackq.q_tail = 
					vcir->lvc_xmtq.q_tail;
			}

			vcir->lvc_xmtq = vcir->lvc_ackq;
			vcir->lvc_ackq.q_head = vcir->lvc_ackq.q_tail = 0;
		}
		latsend(vcir);
	    }
	    else {

	/* We have not had any response to any of our messages
	 * through several timeouts, terminate the virtual
	 * circuit.
	 */
		terminatevc(vcir, STOP_RETRANSMIT);
	    }

} /* rxmit_ackq */
