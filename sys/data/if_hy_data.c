
/*
 * static	char	*sccsid = "@(#)if_hy_data.c	4.1	(ULTRIX)	7/2/90"
 */

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
 *    R. Bhanukitsiri - 02/06/89                                        *
 *            Reflect V3.2 source pool changes.                         *
 *                                                                      *
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/

#include "hy.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/ioctl.h"

#include "../net/net/if.h"
#include "../net/net/netisr.h"
#include "../net/net/route.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_var.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"

#include "../machine/cpu.h"
#include "../machine/mtpr.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"


#include "../io/netif/if_hy.h"
#include "../io/netif/if_hyreg.h"
#include "../io/netif/if_uba.h"

#ifdef BINARY
struct	uba_device *hyinfo[];
#else
struct	uba_device *hyinfo[NHY];
#endif

/*
 * Hyperchannel software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * hy_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * We also have, for each interface, a UBA interface structure, which
 * contains information about the UNIBUS resources held by the interface:
 * map registers, buffered data paths, etc.  Information is cached in this
 * structure for use by the if_uba.c routines in running the interface
 * efficiently.
 */
#ifdef BINARY
struct	hy_softc {
	struct	ifnet hy_if;		/* network-visible interface */
	struct	ifuba hy_ifuba;		/* UNIBUS resources */
	short	hy_flags;		/* flags */
	short	hy_state;		/* driver state */
	u_short	hy_host;		/* local host number */
	struct	in_addr hy_addr;	/* internet address */
	int	hy_olen;		/* packet length on output */
	int	hy_lastwcr;		/* last command's word count */
	short	hy_savedstate;		/* saved for reissue after status */
	short	hy_savedcmd;		/* saved command for reissue */
	int	hy_savedcount;		/* saved byte count for reissue */
	int	hy_savedaddr;		/* saved unibus address for reissue */
	int	hy_ntime;		/* number of timeouts since last cmd */
	int	hy_retry;		/* retry counter */
	struct	hy_stat hy_stat;	/* statistics */
	struct	hy_status hy_status;	/* status */
} hy_softc[];

extern	struct	uba_device *hyinfo[];
extern	int	nNHY;

#ifdef HYROUTE
struct hy_route hy_route[];
#endif

#else
struct	hy_softc {
	struct	ifnet hy_if;		/* network-visible interface */
	struct	ifuba hy_ifuba;		/* UNIBUS resources */
	short	hy_flags;		/* flags */
	short	hy_state;		/* driver state */
	u_short	hy_host;		/* local host number */
	struct	in_addr hy_addr;	/* internet address */
	int	hy_olen;		/* packet length on output */
	int	hy_lastwcr;		/* last command's word count */
	short	hy_savedstate;		/* saved for reissue after status */
	short	hy_savedcmd;		/* saved command for reissue */
	int	hy_savedcount;		/* saved byte count for reissue */
	int	hy_savedaddr;		/* saved unibus address for reissue */
	int	hy_ntime;		/* number of timeouts since last cmd */
	int	hy_retry;		/* retry counter */
	struct	hy_stat hy_stat;	/* statistics */
	struct	hy_status hy_status;	/* status */
} hy_softc[NHY];

struct	uba_device *hyinfo[NHY];
int	nNHY = NHY;

#ifdef HYROUTE
struct hy_route hy_route[NHY];
#endif

#endif

