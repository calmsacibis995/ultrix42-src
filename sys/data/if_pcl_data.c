/*
 * @(#)if_pcl_data.c	4.1	(ULTRIX)	7/2/90
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

#include "pcl.h"

/*
 * DEC CSS PCL-11B Parallel Communications Interface
 *
 * Written by Mike Muuss and Jeff Schwab.
 */

/************************************************************************
 *			Modification History				*
 *									*
 * 03-Feb-89	R. Bhanukitsiri						*
 *	Reflect V3.2 source pool changes.				*
 *									*
 ************************************************************************/

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/buf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/errno.h"

#include "../net/net/if.h"
#include "../net/net/netisr.h"
#include "../net/net/route.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"

#include "../machine/cpu.h"
#include "../machine/mtpr.h"
#include "../io/netif/if_pclreg.h"
#include "../io/netif/if_uba.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

/*
 * PCL software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * sc_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * We also have, for each interface, a UBA interface structure, which
 * contains information about the UNIBUS resources held by the interface:
 * map registers, buffered data paths, etc.  Information is cached in this
 * structure for use by the if_uba.c routines in running the interface
 * efficiently.
 */

#ifdef	BINARY

struct	pcl_softc {
	struct	ifnet sc_if;		/* network-visible interface */
	struct	ifuba sc_ifuba;		/* UNIBUS resources */
	short	sc_oactive;		/* is output active? */
	short	sc_olen;		/* length of last output */
	short	sc_lastdest;		/* previous destination */
	short	sc_odest;		/* current xmit destination */
	short	sc_bdest;		/* buffer's stated destination */
	short	sc_pattern;		/* identification pattern */
} pcl_softc[];


struct	uba_device	*pclinfo[];
extern	int	nNPCL;

#else

struct	pcl_softc {
	struct	ifnet sc_if;		/* network-visible interface */
	struct	ifuba sc_ifuba;		/* UNIBUS resources */
	short	sc_oactive;		/* is output active? */
	short	sc_olen;		/* length of last output */
	short	sc_lastdest;		/* previous destination */
	short	sc_odest;		/* current xmit destination */
	short	sc_bdest;		/* buffer's stated destination */
	short	sc_pattern;		/* identification pattern */
} pcl_softc[NPCL];


struct	uba_device	*pclinfo[NPCL];

int	nNPCL = NPCL;
#endif
