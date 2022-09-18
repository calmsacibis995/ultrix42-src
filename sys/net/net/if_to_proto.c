#ifndef lint
static	char	*sccsid = "@(#)if_to_proto.c	4.2	(ULTRIX)	9/4/90";
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
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../net/net/af.h"
#include "../net/net/if.h"
#include "../net/net/if_to_proto.h"
#include "../net/netinet/in.h"
#include "../net/netinet/if_ether.h"

/*
 * -given the link level protocol type, find the corresponding protocol family. 
 * -return the switch table entry corresponding to the protocol family. 
 * -return 0 if protocol not found.
 * 
 */
struct protosw *
iftype_to_proto(type)
register int type;
{
	register struct if_family *i;
	for (i=if_family; i->domain != -1; i++)
		if (i->if_type == -1 || i->if_type == type)
			return(i->prswitch);
	return(0);
}


/*
 * -given the address family (domain), find the corresponding protocol family. 
 * -return the switch table entry corresponding to the protocol family. 
 * -return 0 if protocol not found.
 * 
 */
struct protosw *
iffamily_to_proto(family)
register int family;
{
	register struct if_family *i;
	for (i=if_family; i->domain != -1; i++)
		if (i->domain == family)
			return(i->prswitch);
	return(0);
}



if_to_proto_init()
{
	register struct if_family *i;
	for (i=if_family; i->domain != -1; i++)
		i->prswitch = pffindproto(i->domain, i->proto, 0);
}
