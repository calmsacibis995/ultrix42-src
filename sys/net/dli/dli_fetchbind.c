#ifndef	lint
static char *sccsid = "@(#)dli_fetchbind.c	4.1	ULTRIX	7/2/90";
#endif	lint

/*
 * Program dli_fetchbind.c,  Module DLI 
 *
 * Copyright (C) 1985 by
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
 *		d l i _ f e t c h b i n d
 *
 * Return current value of info bound to socket.
 *
 * Returns:		Error code if error occurs, otherwise NULL.
 *			Bind info  and length.
 *
 * Inputs:
 *	uentry		= Pointer to the user's line table entry for this request.
 *	so_addr		= Pointer to structure containing address info.
 *	so_addrlen	= Pointer length of structure returned.
 */
dli_fetchbind( uentry, so_addr, so_addrlen )
register struct dli_line *uentry;
register struct sockaddr_dl *so_addr;
register short *so_addrlen;
{
	register int i = 0;
	register struct sockaddr_dl *lineid;

	lineid = &uentry->dli_lineid;

	if ( *so_addrlen < sizeof(struct sockaddr_dl) )
	{
		return(EFAULT);
	}


	switch (lineid->dli_substructype)
	{
		case DLI_802:
		case DLI_ETHERNET:
		case DLI_POINTOPOINT:
			*so_addrlen = sizeof(struct sockaddr_dl);
			bcopy((u_char *) lineid, (u_char *) so_addr, *so_addrlen);
			break;

		default:
			return(EOPNOTSUPP);
			break;
	}
	return(NULL);

}



