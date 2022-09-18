#ifndef lint
static	char	*sccsid = "@(#)if_to_proto_data.c	4.2	(ULTRIX)	9/4/90";
#endif /* lint */
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
 *    R. Bhanukitsiri - 02/06/89					*
 *            Reflect V3.2 source pool changes.				*
 *									*
 *	Larry Palmer - 13-Apr-88 
 *		Fixed an omission in appletalk.
 *	Larry Palmer - 15-Jan-88					*
 *		Added appletalk and xns hooks.				*
 *									*
 *	U. Sinkewicz - 08-19-87						*
 *		Backed out unnecessary BSC info.			*
 *	U. Sinkewicz - 03/12/86						*
 *		Added BSC info 2780/3780 Emulator			*
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


#define	IFNULL \
	{ 0,	0,	0,	0 }

#define IFEND \
	{ -1,	-1,	-1,	0}


#ifdef INET
#include "inet.h"
/*
 * need an entry for each device that has a data type field. 
 * ethernets and hyperchannels are examples
 */
#define	ETHER_IP  \
	{ ETHERTYPE_IP,	AF_INET,	IPPROTO_IP,	0 }
#endif



#if defined(DECNET) && !defined(OSI)
#include "decnet.h"
#include "../net/netdnet/dn.h"
#define	ETHER_DECNET  \
	{ ETHERTYPE_DN,	AF_DECnet,	DNPROTO_NSP,	0 }
#endif

#ifdef LAT
#include "lat.h"
#if NLAT == 1
#define ETHER_LAT \
	{ ETHERTYPE_LAT,	AF_LAT,		0,		0 }
#endif
#endif

#ifdef APPLETALK
#include "appletalk.h"
#define ETHER_APPLE \
	{ETHERTYPE_ATALK,	AF_APPLETALK,	PF_APPLETALK,		0}
#define ETHER_APPLEARP \
	{ETHERTYPE_AARP,	AF_APPLETALK,	PF_APPLETALK,		0}
#endif

#ifdef NS
#include "xns.h"
#include "../net/netns/ns.h"
#define ETHER_NS \
	{ETHERTYPE_NS,		AF_NS,	NSPROTO_SPP,	0}
#endif NS

/*
 * The DLI entry should be the last one in the table since it will be the
 * destination for all packets which do not match any earlier entries.
 */
#ifdef DLI
#include "dli.h"
#define ETHER_DLI \
	{ -1,			AF_DLI,		0,		0 }
#else
#include "dli.h"
#define ETHER_DLI \
        { ETHERTYPE_LOOP,       AF_DLI,         0,              0 }, \
        { ETHERTYPE_RC,         AF_DLI,         0,              0 }
#endif

#ifdef	BINARY

extern struct if_family if_family[];

#else

/* INET specific stuff is kept in drivers for now */

struct if_family if_family[] = {
#ifdef ETHER_DECNET
	ETHER_DECNET,
#endif
#ifdef ETHER_LAT
	ETHER_LAT,
#endif
#ifdef ETHER_APPLE
	ETHER_APPLE,
#endif
#ifdef ETHER_APPLEARP
	ETHER_APPLEARP,
#endif
#ifdef ETHER_NS
	ETHER_NS,
#endif
#ifdef ETHER_DLI
	ETHER_DLI,
#endif
	IFEND
};

#endif
