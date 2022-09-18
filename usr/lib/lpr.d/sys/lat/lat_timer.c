#ifndef lint
static char *sccsid = "@(#)lat_timer.c	4.1.1.3	2/29/88";
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
 *      Chung Wong - 1/7/88                                             *
 *              Added 'wakeup(solptr->sol1_solid)' for host initiated   *
 *              connection.                                             * 
 *		Clean response queue when solicit timer expires.	*
 *                                                                      *
 *	Peter Harbo - 4/15/86						*
 * 		Changes in lat_slowtimo() to resend unacknowledged	*
 *		solicit information messages.				*
 *									*
 ************************************************************************/


/*	lat_timer.c	0.0	11/9/84	*/
/*	lat_timer.c	1.0	4/15/86 */

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

extern struct lat_vc *vc[];
extern u_char lattmoactive;
extern struct lat_counters latctrs;
extern u_short soltimer, solxmit;
extern struct sclass class1;
extern struct lataddr LATmcast;
extern struct socket *lat_traceso;


/*
 * LAT timer routines.
 */

/*
 *		l a t _ s l o w t i m o
 *
 * LAT slow timeout. This routine is entered every 500ms.
 *
 * Outputs:		None.
 *
 * Inputs:		None.
 */
lat_slowtimo()
{
    register struct lat_vc *vcir;
    register int i;
    int s = splnet();
    struct ifnet *ifp = ifnet;
    struct mbuf *m,*sm;

    lattmoactive++;
    for (i = 1; i < LAT_MAXVC; i++)
    {
	if (vcir = vc[i])
	{
	    if (vcir->lvc_xmtq.q_head)
	    {
		latsend(vcir);
	    }
	    /*
	     * If the resource recovery timer is running and expires, try
	     * send another run message on the virtual circuit.
	     */
	    if (vcir->lvc_resource && --vcir->lvc_resource == 0)
	    {
		vcrun(vcir);
	    }
	    /*
	     * If the acknowledgement timer is running and expires, move
	     * the pending acknowldgement queue to the front of the
	     * transmit queue and initiate transmission.
	     */
	    if (vcir->lvc_timer && --vcir->lvc_timer == 0)
	    {
		rxmit_ackq(vcir);
	    }
            lat_alive(vcir); /* latmaster */
	}
    }

    if (class1.scl_smsg.q_head)
        for(m = class1.scl_smsg.q_head ; m ; m = m->m_act )
        {
    	    char *soltimer1 = mtod(m, char *) + m->m_len;
	    char *solxmit1 = mtod(m, char *) + m->m_len + 1;

	    if (*soltimer1 && --*soltimer1 == 0)
	    {
	        if (--*solxmit1)
	        {
	            if (lat_traceso)
		        ltt_trace(0,0,m,LATmcast.lat_addr);
	            if (sm = m_copy(m,0,M_COPYALL))
		        (*ifp->if_output)(ifp,sm,&LATmcast);
                    *soltimer1 = LAT_SOLTIMER * 2;
	        }
	        else
	        {
                    struct solicit_1 *solptr; 

	            DEQUEUE(&(class1.scl_smsg),m);
                    solptr = mtod(m, struct solicit_1 *); 
                    wakeup(solptr->sol1_solid); 
		    m_freem(m);
	        }
	    }
	}

    if (soltimer) soltimer--;
    if (!soltimer && (m = class1.scl_rmsg.q_head))
    {
	/* 
         * Due to the SIOCGIFCONF interface, application (lat_conn) may
  	 * send out more than 1 solicit message over a device and get 
	 * multiple responses.  Since the application only takes the first
 	 * response, the duplicated response would stay on the queue.
  	 * Assuming nobody clears solicit timer, when it expires,
         * no one should still expect response message.  Clean up the queue.
	 */
	while (m)
	{
	    sm = m->m_act;
	    m_freem(m);
	    m = sm;
	}
	class1.scl_rmsg.q_head = class1.scl_rmsg.q_tail = 0;
    }

    lattmoactive--;
    splx(s);
}
