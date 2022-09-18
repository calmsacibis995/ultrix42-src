#ifndef lint
static char *sccsid = "@(#)ltt_usrreq.c	4.2	2/29/88";
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
 *	Chung Wong  - 1/7/88						*
 *		Use PRU_BIND to turn on multicast receive mode.         *
 *		Turn off multicast receive mode in PRU_DETACH.          *
 *									*
 ************************************************************************/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/ioctl.h"


#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#include "../lat/lat.h"
#include "../lat/lat_protocol.h"
#include "../lat/lat_var.h"

struct socket *lat_traceso = 0;


/*
 *			      l t t _ u s r r e q
 *
 * Process a trace socket user request.
 *
 * Returns:		User error code
 *			0 if request was successful
 *
 * Inputs:
 * 	so	      = Pointer to the socket for this request
 *	req	      = Request function code
 *      m	      = Pointer to MBUF chain for data
 *	nam	      = Pointer to MBUF chain for addressing
 *	rights	      = Pointer to MBUF chain for access rights
 */
/*ARGSUSED*/
ltt_usrreq(so,req,m,nam,rights)
register struct socket *so;
int req;
struct mbuf *m,*nam,*rights;
{
    int error = 0;
    int s = splnet();

    switch(req)
    {
	case PRU_ATTACH:

	/* The ATTACH function is called as a result of a user process 
	 * issuing a socket() call.  Only a super-user is allowed to open the
	 * trace socket.
	 */

	    if (suser())
	    {
		if (lat_traceso == 0)
		{
		    if ((error = soreserve(so,0,LATBUFF_TRACE)) == 0)
		    {
			lat_traceso = so;
			so->so_pcb = (caddr_t)-1;
			soisconnected(so);
			break;
		    }
		}
	    }
	    else error = EACCES;
	    break;

	case PRU_DETACH:
	/* The DETACH function is called as a result of the last close()
	 * call on a socket.
	 */

            lat_multi(0); 
	    soisdisconnected(so);
	    so->so_pcb = 0;
	    lat_traceso = 0;
	    break;

	case PRU_BIND:
            lat_multi(1);
            break;

	/* All other requests are unsupported. */

	default:
	    error = EOPNOTSUPP ; 
	    break;

    }
    splx(s);
    return(error);

}
