/*
 * static	char	*sccsid = "@(#)if_ec_data.c	4.1	(ULTRIX)	7/2/90";
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
 *
 * 24-Mau-89 -- darrell
 *		Changed the #include for cpuconf.h to find it in it's
 *		new home -- sys/machine/common/cpuconf.h
 *
 * 05-Feb-89 -- R. Bhanukitsiri
 *                 Reflect V3.2 source pool changes.
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 ************************************************************************/

#include "ec.h"


/*
 * 3Com Ethernet Controller interface
 */
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
#include "../../machine/common/cpuconf.h"

#include "../net/net/if.h"
#include "../net/net/netisr.h"
#include "../net/net/route.h"
#include "../net/netinet/in.h"
#include "../net/netinet/in_var.h"
#include "../net/netinet/in_systm.h"
#include "../net/netinet/ip.h"
#include "../net/netinet/ip_var.h"
#include "../net/netinet/if_ether.h"

#ifdef NS
#include "../netns/ns.h"
#include "../netns/ns_if.h"
#endif

#include "../machine/cpu.h"
#include "../machine/mtpr.h"
#include "../io/netif/if_ecreg.h"
#include "../io/netif/if_uba.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * es_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 * We also have, for each interface, a UBA interface structure, which
 * contains information about the UNIBUS resources held by the interface:
 * map registers, buffered data paths, etc.  Information is cached in this
 * structure for use by the if_uba.c routines in running the interface
 * efficiently.
 */
#define	es_if	es_ac.ac_if		/* network-visible interface */
#define	es_addr	es_ac.ac_enaddr		/* hardware Ethernet address */

#ifdef	BINARY

extern	struct	ec_softc {
	struct	arpcom es_ac;		/* common Ethernet structures */
	struct	ifuba es_ifuba;		/* UNIBUS resources */
	short	es_mask;		/* mask for current output delay */
	short	es_oactive;		/* is output active? */
	u_char	*es_buf[16];		/* virtual addresses of buffers */
	u_short es_unrecog;		/* unrecognized frame destination */
} ec_softc[];

extern	struct	uba_device *ecinfo[];

extern	int	nNEC;

#else

struct	ec_softc {
	struct	arpcom es_ac;		/* common Ethernet structures */
	struct	ifuba es_ifuba;		/* UNIBUS resources */
	short	es_mask;		/* mask for current output delay */
	short	es_oactive;		/* is output active? */
	u_char	*es_buf[16];		/* virtual addresses of buffers */
	u_short es_unrecog;		/* unrecognized frame destination */
} ec_softc[NEC];

struct	uba_device *ecinfo[NEC];

int	nNEC = NEC;

#endif
