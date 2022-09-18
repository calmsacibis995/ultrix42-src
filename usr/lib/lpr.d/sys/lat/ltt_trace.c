#ifndef lint
static char *sccsid = "@(#)ltt_trace.c	4.2	2/29/88";
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
 * Revision 1.0.	Peter Harbo 9/9/86				*
 * 		First version						*
 *									*
 *	Chung Wong - 1/7/88						*
 *		Add direction bit in trace (ttype).                     *
 *									*
 ************************************************************************/

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

static int lat_drop;
extern struct socket *lat_traceso;


/*
 *			       l t t _ t r a c e
 *
 * Trace LAT protocol.  Allocate a trace record and append it to the trace
 * socket receive buffer.
 *
 * Returns:	Nothing.
 *
 * Inputs:
 *	ttype		= Type of trace entry
 *  			  low order bit- msg direction 0:outbound 1:inbound
 *	usrreq		= Type of user request
 *	m		= Mbuf chain to be traced
 *	caller		= Name of calling routine
 */
/*ARGSUSED*/
ltt_trace(ttype,usrreq,m,addr)
int ttype,usrreq;
struct mbuf *m;
char *addr;
{
    register struct mbuf *n, *page;
    caddr_t addrp;

#ifdef notdef
    if ( valid_chain(m,caller) )
    {
	if ( (n = m_copy(m,0,M_COPYALL)) == 0 )
	{
	    lat_drop++;
	    return;
	}
    }
#endif

    if (n = m_get(M_DONTWAIT,MT_DATA))
    {
	MCLGET(n,page);
	if (page)
	{
	    n->m_len = 6;
	    addrp = mtod(n, caddr_t);
	    bcopy(addr,addrp,6);
            *(addrp+6) = ttype & 0x01; /* message direction */
            n->m_len++;                
	    if ( ( n->m_next = m_copy((struct mbuf *)m,0,M_COPYALL)) == 0)
	    {
		m_free(n);
		return;
	    }
	    sbappendrecord(&lat_traceso->so_rcv,n);
	    sorwakeup(lat_traceso);
	    return;
	}
	m_free(n);
    }
}

valid_chain(m,caller)
register struct mbuf *m;
register char *caller;
{
    register struct mbuf *tm = m;

    while (tm)
    {
	if (tm->m_type == MT_FREE)
	{
	    printf("%s: mbuf was previously freed\n",caller);
	    return(0);
	}
	tm = tm->m_next;
    }
    return(1);

}
