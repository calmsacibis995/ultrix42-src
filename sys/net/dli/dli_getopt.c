#ifndef	lint
static char *sccsid = "@(#)dli_getopt.c	4.1	ULTRIX	7/2/90";
#endif	lint

/*
 * Program dli_getopt.c,  Module DLI 
 *
 * Copyright (C) 1985, 1988 by
 * Digital Equipment Corporation, Maynard, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 10-Jul-1985
 *      DECnet-ULTRIX   V1.0
 *
 * 2.01 18-Mar-1988
 *      DECnet-ULTRIX   V2.4
 *		- Allowed use of reserved bit in individual and group SAPs
 *
 *
 */

#include "../../h/param.h"
#include "../../h/systm.h"
#include "../../h/mbuf.h"
#include "../../h/socket.h"
#include "../../h/socketvar.h"
#include "../../h/protosw.h"
#include "../../h/errno.h"
#include "../../h/smp_lock.h"
#include "../../h/cpudata.h"

#include "../../net/net/if.h"


#include "../../net/netinet/in.h"
#include "../../net/netinet/if_ether.h"

#include "../../net/netdnet/dli_var.h"






/*
 *		d l i _ g e t o p t
 *
 * Return requested option.
 *
 * Returns:		Error code if error, otherwise NULL.
 *			length of data in option buffer.
 *
 * Inputs:
 *	uentry		= Pointer to the user's line table entry for this request.
 *	optbuf		= Buffer containing option.
 *	optlen		= Pointer to length of data in option buffer.
 *	optnam		= Name of option.
 */
dli_getopt( uentry,optbuf,optlen,optnam ) 
register struct dli_line *uentry;
register u_char *optbuf;
short *optlen;
int optnam;
{
	short i, gsap, cnt;
	

	/*
	 * fetch option.
	 */
	switch (optnam)
	{
		/*
		 * return list of group saps 
		 * that this isap has enabled
		 */
		case DLI_GETGSAP:
			cnt = 0;
			for(gsap = 3; gsap < 256; gsap += 2)
			{
				switch(osi_tst_802gsap(uentry->dli_if, gsap, uentry->dli_lineid.choose_addr.dli_802addr.eh_802.ssap))
				{
					case 0:                 /* gsap not set */
						break;
					case 1:                 /* save gsap */
						optbuf[cnt++] = (u_char)gsap; 
						break;
					case -1:                /* couldn't find the device */
						return(ENOTFND);
						break;
				}
			}
			*optlen = cnt;
			break;

		case DLI_STATE:
			*optbuf = uentry->dli_sockopt.dli_state;
			*optlen = sizeof(u_char);
			break;


		case DLI_INTERNALOOP:
			*optbuf = uentry->dli_sockopt.dli_iloop;
			*optlen = sizeof(u_char);
			break;


		case DLI_MULTICAST:
			/*
			 * invalid for point to point links
			 */
			if ( uentry->dli_lineid.dli_substructype )
			{
				return(ENOPROTOOPT);
			}

			for ( i = 0; i < MCAST_MAXNUM; i++ )
			{
				if( ! (uentry->dli_sockopt.dli_mcast[i][0] & 1) )
				{
					break;
				}
			}
			*optlen = (short) (i * MCAST_SIZE);
			if ( *optlen != NULL )
			{
				bcopy(uentry->dli_sockopt.dli_mcast, optbuf, *optlen);
			}
			break;

		default:
			return(ENOPROTOOPT);
			break;
	}
	return(NULL);
}
